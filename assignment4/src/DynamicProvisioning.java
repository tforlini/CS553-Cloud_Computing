import com.amazonaws.regions.Regions;
import com.amazonaws.services.sqs.AmazonSQSClient;
import com.amazonaws.services.sqs.model.GetQueueAttributesRequest;

import java.util.Map;
import java.util.logging.Logger;

import static java.util.logging.Level.ALL;


public class DynamicProvisioning extends ProvisioningModule {
    private static final Logger logger = Logger.getLogger(DynamicProvisioning.class.getName());

    static {
        logger.setLevel(ALL);
    }

    private String queueName;
    private String queueUrl;
    private int messageCount = 0;
    private AmazonSQSClient sqs;

    /**
     * Build a dynamic provisioning manager by giving the queue name to manage.
     * By default, the manager uses a refresh rate of 5 seconds, runs workers on t2.micro with the AMI ami-9d2d7aad with
     * a time out of 30 seconds.
     *
     * @param queueName name of the queue to manage
     */
    public DynamicProvisioning(String queueName) {
        super();
        this.queueName = queueName;
        this.killTime = 30;
    }


    protected void provisionWorkers() {
        sqs = new AmazonSQSClient(credentials);
        sqs.setRegion(Regions.US_WEST_2);
        queueUrl = sqs.getQueueUrl(queueName).getQueueUrl();

        Map<String, String> attributes = sqs.getQueueAttributes(new GetQueueAttributesRequest(queueUrl).withAttributeNames("ApproximateNumberOfMessages")).getAttributes();
        int newMessageCount = Integer.parseInt(attributes.get("ApproximateNumberOfMessages"));
        int diff = newMessageCount - messageCount;
        if (diff > 0) {
            logger.info("Launching new worker instance [" + diff + " new messages].");
            addWorker();
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
            }
        }
        messageCount = newMessageCount;
    }
}
