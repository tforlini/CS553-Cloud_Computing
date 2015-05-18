import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.profile.ProfileCredentialsProvider;
import com.amazonaws.regions.Region;
import com.amazonaws.regions.Regions;
import com.amazonaws.services.ec2.AmazonEC2;
import com.amazonaws.services.ec2.AmazonEC2Client;
import com.amazonaws.services.ec2.model.TerminateInstancesRequest;
import com.amazonaws.services.sqs.AmazonSQSClient;
import com.amazonaws.services.sqs.model.DeleteMessageRequest;
import com.amazonaws.services.sqs.model.Message;
import com.amazonaws.services.sqs.model.ReceiveMessageRequest;
import com.amazonaws.services.sqs.model.SendMessageRequest;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.ConsoleHandler;
import java.util.logging.Logger;

import static java.util.logging.Level.ALL;

public class SQSWorker {
    private static final Logger logger = Logger.getLogger(SQSWorker.class.getName());
    private static final ConsoleHandler handler = new ConsoleHandler();

    static {
        logger.setLevel(ALL);
    }

    private static int maxIdleTime;
    private static AmazonSQSClient sqs;
    private static final int SQS_CALL_WAIT = 1;

    /**
     * Print the cli usage for the worker.
     */
    private static void printUsage() {
        logger.severe("Usage:\n" +
                "worker -i <TIME_SEC>");
    }

    /**
     * Parse the arguments given to run the process.
     * @param args list of arguments
     */
    private static void parseArgs(String[] args) {
        if ("-i".equals(args[0])) {
            try {
                maxIdleTime = Integer.parseInt(args[1]);
                maxIdleTime *= 1000; // time in seconds
            } catch (Exception e) {
                printUsage();
                System.exit(1);
            }
        } else {
            printUsage();
            System.exit(1);
        }
    }

    public static void main(String[] args) {
        parseArgs(args);
        AWSCredentials credentials = null;
        try {
            credentials = new ProfileCredentialsProvider().getCredentials();
        } catch (Exception e) {
            e.printStackTrace();
        }
        sqs = new AmazonSQSClient(credentials);
        Region usWest2 = Region.getRegion(Regions.US_WEST_2);
        sqs.setRegion(usWest2);

        //get tasks and result Queue URLs
        String clientQueueUrl = sqs.createQueue("clientQueue").getQueueUrl();
        String resultQueueUrl = sqs.createQueue("resultQueue").getQueueUrl();

        logger.info("client queue: " + clientQueueUrl);
        logger.info("result queue: " + resultQueueUrl);

        long idleBegin = System.currentTimeMillis();
        long idle;
        Task task;
        TaskTable taskTable = new TaskTable("DynamoDB");
        taskTable.createTable();

        while ((idle = System.currentTimeMillis() - idleBegin) < maxIdleTime) {
            try {
                logger.fine("Idle time: " + idle);
                ReceiveMessageRequest request = new ReceiveMessageRequest(clientQueueUrl);
                request.withWaitTimeSeconds(SQS_CALL_WAIT).setMaxNumberOfMessages(1);
                List<Message> messages = sqs.receiveMessage(request).getMessages();

                if (!messages.isEmpty()) {
                    sqs.deleteMessage(new DeleteMessageRequest(clientQueueUrl, messages.get(0).getReceiptHandle()));
                    String msg = messages.get(0).getBody();
                    task = new Task(msg);
                    if (taskTable.updateTask(task.getId(), 1)) {
                        logger.info("Handling task #" + task.getId() + ".");
                        task.run();
                        String result = task.getId() + " " + task.getResult();
                        logger.fine("Result " + result + " sent to SQS.");
                        sqs.sendMessage(new SendMessageRequest(resultQueueUrl, result));
                        idleBegin = System.currentTimeMillis();
                    } else {
                        logger.info("Task " + task.getId() + " has already been completed.");
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        try {
            AmazonEC2 ec2 = new AmazonEC2Client(credentials);
            ec2.setRegion(usWest2);

            URL ec2Url = new URL("http://169.254.169.254/latest/meta-data/instance-id");
            URLConnection ec2Connection = ec2Url.openConnection();
            BufferedReader in = new BufferedReader(new InputStreamReader(ec2Connection.getInputStream()));
            List<String> ec2id = new ArrayList<String>();
            ec2id.add(in.readLine());

            // Terminate instance
            TerminateInstancesRequest terminateRequest = new TerminateInstancesRequest(ec2id);
            ec2.terminateInstances(terminateRequest);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
