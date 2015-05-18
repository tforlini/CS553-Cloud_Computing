# CS553 - Programming Assignment 4

## Thomas Dubucq - Tony Forlini - Virgile Landeiro Dos Reis

### Dependencies

This programming assignment has been developed in Java 7 and uses [Ant](http://ant.apache.org/) to build the project.

The [AWS SDK](http://aws.amazon.com/sdk-for-java/) is also required to deploy this project as the project automatically run and terminate EC2 instances.

The AMIs used are the following:

- worker: ami-af6f389f
- client and scheduler: ami-f36f38c3

### Deploy

Here are the instructions to follow in order to deploy the app:

  - Create a AWS security group that authorizes the connection on the port you want to use. If you do not know what port you are going to use, just create a security group that authorizes all the connection. Name this security group "assignment4".
  - Launch 2 t2.micro instances with the AMI ami-f36f38c3 and put them in the security group created previously "assignment4"). One instance is made to run the client, the other is made to run the scheduler so feel free to rename them in the AWS console.
  - In the scheduler, several commands are available:

```bash
# run k local workers, client will connect to the port 1234
scheduler -s 1234 -lw k
# run k remote workers using static provisioning, client will connect to the port 1234
scheduler -s 1234 -rw k
# run remote workers using dynamic provisioning, client will connect to the port 1234
scheduler -s 1234 -rw
```
  
  - Run one of these commands to launch a scheduler.
  - Before running anything on the client, copy the public IP address of the scheduler instance (called scheduler_ip in the following).
  - To run the client process from the client instance, use the following command:
  
```bash
# run a given workload
client -s scheduler_ip:1234 -w /path/to/workload
```
  
  - The workloads are stored in ~/workloads on the instance.
  
  
