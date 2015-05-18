import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.profile.ProfileCredentialsProvider;
import com.amazonaws.regions.Regions;
import com.amazonaws.services.sqs.AmazonSQSClient;
import com.amazonaws.services.sqs.model.DeleteMessageRequest;
import com.amazonaws.services.sqs.model.Message;
import com.amazonaws.services.sqs.model.ReceiveMessageRequest;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.logging.ConsoleHandler;
import java.util.logging.Logger;

import static java.util.logging.Level.ALL;

public class Scheduler {
    private static final Logger logger = Logger.getLogger(Scheduler.class.getName());
    private static final ConsoleHandler handler = new ConsoleHandler();

    static {
        logger.setLevel(ALL);
        //handler.setLevel(INFO);
        //logger.addHandler(handler);
    }

    private static final int LOCAL = 1;
    private static final int REMOTE = 2;
    private static final int MAX_WORKERS = 32;
    private static final int SQS_CALL_WAIT = 1;

    private static int workersType;
    private static int lwQuantity;
    private static int portNumber;
    private static int ntasks;
    private static boolean useDynamicProvisioning;
    private static int staticProvisioningCount;

    private static AmazonSQSClient sqs;
    private static BufferedReader reader;
    private static PrintWriter writer;
    private static ArrayList<Task> resultsList;

    /**
     * Get the tasks to process from the client and store them in a thread safe queue.
     * @param reader stream reader to receive messages from the client
     * @param writer stream writer to send messages to the client
     * @return a thread safe queue containing the tasks to process
     * @throws IOException
     */
    private static BlockingQueue<Task> getTasks(BufferedReader reader, PrintWriter writer) throws IOException {
        String str;
        BlockingQueue<Task> tasks = new LinkedBlockingQueue<Task>();
        logger.info("Receiving tasks from client...");
        while ((str = reader.readLine()) != null) {
            if ("done".equals(str))
                break;
            tasks.add(new Task(str));
            ntasks++;
        }
        logger.info(ntasks + " tasks received");
        writer.println("tasks received");
        return tasks;
    }

    /**
     * Parse the command line interface arguments to launch the scheduler.
     * @param args list of arguments passed to the cli
     */
    private static void parseArgs(String[] args) {
        logger.info("Launching the tasks scheduler.");
        boolean cond = "-s".equals(args[0]) && ("-lw".equals(args[2]) || "-rw".equals(args[2]));
        if (!cond) {
            printUsage();
        } else {
            if ("-lw".equals(args[2])) {
                workersType = LOCAL;
                lwQuantity = Integer.parseInt(args[3]);
                logger.info("Using " + lwQuantity + " local workers.");
            } else if ("-rw".equals(args[2])) {
                workersType = REMOTE;
                logger.info("Using remote workers.");
                useDynamicProvisioning = true;
                staticProvisioningCount = 0;
                if (args.length == 4) {
                    try {
                        staticProvisioningCount = Integer.parseInt(args[3]);
                        useDynamicProvisioning = false;
                    } catch (NumberFormatException e) {
                    }
                }
                if (useDynamicProvisioning)
                    logger.info("Using dynamic provisioning.");
                else
                    logger.info("Using static provisioning with " + staticProvisioningCount + " workers.");
            } else {
                printUsage();
            }
        }
        portNumber = Integer.parseInt(args[1]);
    }

    /**
     * Intern class that handles the polling from the client. It also sends the results back to the client.
     */
    private static class PollThread extends Thread {
        @Override
        public void run() {
            logger.fine("PollThread running!");
            int tasks_sent = 0;
            int tasks_stored = 0;
            String line = null;
            try {
                while (tasks_sent < ntasks && null != (line = reader.readLine())) {
                    logger.fine("readline: " + line);
                    logger.fine("resultsList: " + resultsList.size() + " elements/" + ntasks + " tasks");
                    String[] args = line.split(" ");
                    String tosend = "";
                    if ("poll".equals(args[0])) {
                        if (tasks_stored == resultsList.size()) {
                            tosend += "-1";
                        } else {
                            while (tasks_stored < resultsList.size()) {
                                tosend += tasks_stored;
                                tasks_stored++;
                                if (tasks_stored != resultsList.size())
                                    tosend += " ";
                            }
                        }
                    } else if ("get".equals(args[0])) {
                        int result_idx = Integer.parseInt(args[1]);
                        int task_id = resultsList.get(result_idx).getId();
                        int task_result = resultsList.get(result_idx).getResult();
                        tosend = task_id + " " + task_result;
                        tasks_sent++;
                        if (tasks_sent % 500 == 0) {
                            logger.info(tasks_sent + " tasks sent to client.");
                        }
                    }
                    logger.fine(tasks_sent + " sent," + tasks_stored + " stored," + ntasks + " total");
                    writer.println(tosend);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void main(String[] args) {
        parseArgs(args);
        try {
            //Listen on port
            ServerSocket serverSock = new ServerSocket(portNumber);
            logger.info("Scheduler is listening...");

            //Get connection
            Socket clientSock = serverSock.accept();
            logger.info("A client just connected to the scheduler.");

            //Get input
            reader = new BufferedReader(new InputStreamReader(clientSock.getInputStream()));
            writer = new PrintWriter(clientSock.getOutputStream(), true);

            final BlockingQueue tasks = getTasks(reader, writer);

            BlockingQueue results = new LinkedBlockingQueue();
            resultsList = new ArrayList<Task>();
            int tasks_done = 0;

            PollThread pollThread = new PollThread();
            switch (workersType) {
                case LOCAL:
                    ThreadPool pool = new ThreadPool(lwQuantity, tasks, results);
                    pollThread.start();

                    while (tasks_done < ntasks) {
                        Task t = (Task) results.take();
                        resultsList.add(t);
                        logger.fine("Result for task " + t.getId() + ": " + t.getResult() + " (" + tasks_done + "/" + ntasks + ")");
                        tasks_done++;
                    }
                    pollThread.join();
                    break;

                case REMOTE:
                    AWSCredentials credentials = new ProfileCredentialsProvider("default").getCredentials();
                    sqs = new AmazonSQSClient(credentials);
                    sqs.setRegion(Regions.US_WEST_2);

                    final String tasksQueue = sqs.createQueue("clientQueue").getQueueUrl();
                    final String resultsQueue = sqs.createQueue("resultQueue").getQueueUrl();

                    ProvisioningModule provisioningModule;
                    Thread provisioningThread;
                    if (useDynamicProvisioning)
                        provisioningModule = new DynamicProvisioning("clientQueue");
                    else
                        provisioningModule = new StaticProvisioning(staticProvisioningCount);
                    provisioningThread = new Thread(provisioningModule);
                    provisioningThread.start();

                    TaskTable dynamo = new TaskTable("DynamoDB");
                    dynamo.createTable();

                    while (!tasks.isEmpty()) {
                        Task task = null;
                        try {
                            task = (Task) tasks.take();
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        dynamo.setTask(task.getId(), 0);
                        sqs.sendMessage(tasksQueue, task.toString());
                    }

                    provisioningModule.stop();
                    try {
                        provisioningThread.join();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                    logger.info("Tasks sent in queue and set in DynamoDB");

                    pollThread.start();
                    while (tasks_done < ntasks) {
                        ReceiveMessageRequest request = new ReceiveMessageRequest(resultsQueue);
                        request.withWaitTimeSeconds(SQS_CALL_WAIT).setMaxNumberOfMessages(10);
                        List<Message> messages = sqs.receiveMessage(request).getMessages();

                        Task task_result;
                        if (!messages.isEmpty()) {
                            for (Message msg : messages) {
                                sqs.deleteMessage(new DeleteMessageRequest(resultsQueue, msg.getReceiptHandle()));
                                String[] split = msg.getBody().split(" ");
                                int task_id = Integer.parseInt(split[0]);
                                int result = Integer.parseInt(split[1]);
                                task_result = new Task(task_id, null);
                                task_result.setResult(result);
                                resultsList.add(task_result);
                                tasks_done++;
                                if (tasks_done % 500 == 0) {
                                    logger.info(tasks_done + " tasks get from result queue.");
                                }
                                logger.fine("Result for task " + task_result.getId() + ": " + task_result.getResult() + " (" + tasks_done + "/" + ntasks + ")");
                            }
                        } else {
                            Thread.sleep(100);
                        }
                    }
                    pollThread.join();
                    break;
                default:
                    throw new IllegalStateException("Worker type " + workersType + " unsupported.");
            }

            serverSock.close();
            clientSock.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Print the cli usage for the scheduler.
     */
    private static void printUsage() {
        logger.severe("Usage:\n" +
                "scheduler -s <PORT> -lw <count>\n" +
                "scheduler -s <PORT> -rw [count]");
        System.exit(1);
    }
}