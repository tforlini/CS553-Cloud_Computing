import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.profile.ProfileCredentialsProvider;
import com.amazonaws.regions.Regions;
import com.amazonaws.services.ec2.AmazonEC2Client;
import com.amazonaws.services.ec2.model.RunInstancesRequest;
import com.amazonaws.services.ec2.model.RunInstancesResult;
import com.amazonaws.util.Base64;

public abstract class ProvisioningModule implements Runnable {

    protected int refreshRate;
    protected String ami;
    protected String instanceType;
    protected int killTime;
    protected AmazonEC2Client ec2;
    protected volatile boolean run = true;
    protected RunInstancesRequest runInstancesRequest;
    protected AWSCredentials credentials;

    /**
     * Default constructor to build a provisioning module.
     */
    public ProvisioningModule() {
        this.refreshRate = 5000;
        this.ami = "ami-af6f389f";
        this.instanceType = "t2.micro";
        this.credentials = new ProfileCredentialsProvider("default").getCredentials();
        this.killTime = 30; // the instance stops after 30 seconds of inactivity
    }

    /**
     * Set the refresh rate.
     *
     * @param refreshRate new refresh rate in seconds
     * @return a pointer to a ProvisioningModule object so that withXXX functions can be called
     *         in sequence.
     */
    public ProvisioningModule withRefreshRate(int refreshRate) {
        this.refreshRate = refreshRate * 1000;
        return this;
    }

    /**
     * Set the ami id.
     *
     * @param ami new ami id
     * @return a pointer to a ProvisioningModule object so that withXXX functions can be called
     *         in sequence.
     */
    public ProvisioningModule withAmi(String ami) {
        this.ami = ami;
        return this;
    }

    /**
     * Set the instance type.
     *
     * @param instanceType new instance type
     * @return a pointer to a ProvisioningModule object so that withXXX functions can be called
     *         in sequence.
     */
    public ProvisioningModule withInstanceType(String instanceType) {
        this.instanceType = instanceType;
        return this;
    }

    /**
     * Set the time out time.
     *
     * @param killTime new time out time in seconds
     * @return a pointer to a ProvisioningModule object so that withXXX functions can be called
     *         in sequence.
     */
    public ProvisioningModule withKillTime(int killTime) {
        this.killTime = killTime;
        return this;
    }

    /**
     * Run a new worker instance.
     */
    public void addWorker() {
        RunInstancesResult runResult = ec2.runInstances(runInstancesRequest);
    }

    /**
     * Stop the provisioning module.
     */
    public void stop() {
        run = false;
    }

    @Override
    public void run() {
        ec2 = new AmazonEC2Client(credentials);
        ec2.setRegion(Regions.US_WEST_2);
        String initScript = "#!/bin/bash\n" +
                "mkdir /root/.aws\n" +
                "echo \"[default]\" > /root/.aws/credentials\n" +
                "echo \"aws_access_key_id = " + credentials.getAWSAccessKeyId() + "\" >> /root/.aws/credentials\n" +
                "echo \"aws_secret_access_key = " + credentials.getAWSSecretKey() + "\" >> /root/.aws/credentials\n" +
                "echo \"[default]\" > /root/.aws/config\n" +
                "echo \"Worker script is launching...\" > /var/log/sqs_worker.log\n" +
                "ant worker -f /home/ec2-user/cs553-cloudcomputing-2014/assignment4/build.xml -Darg0=-i -Darg1=" + killTime + " &>> /var/log/sqs_worker.log &";
        String base64script = Base64.encodeAsString(initScript.getBytes());
        runInstancesRequest = new RunInstancesRequest()
                .withImageId(ami)
                .withInstanceType(instanceType)
                .withSecurityGroupIds("assignment4")
                .withKeyName("pa4sqs")
                .withMinCount(1)
                .withMaxCount(1)
                .withUserData(base64script);

        while (run) {
            try {
                provisionWorkers();
                Thread.sleep(refreshRate);
            } catch (InterruptedException e) {
                System.out.print("DynamicProvisioning - Error in Thread.sleep\n");
                e.printStackTrace();
            }
        }

    }

    /**
     * Launch the worker provisioning method implemented in the subclass.
     */
    protected abstract void provisionWorkers();
}
