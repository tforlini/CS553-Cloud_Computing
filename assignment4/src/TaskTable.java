import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.profile.ProfileCredentialsProvider;
import com.amazonaws.regions.Regions;
import com.amazonaws.services.dynamodbv2.AmazonDynamoDBClient;
import com.amazonaws.services.dynamodbv2.model.*;
import com.amazonaws.services.dynamodbv2.util.Tables;

import java.util.HashMap;
import java.util.Map;
import java.util.logging.ConsoleHandler;
import java.util.logging.Logger;

import static java.util.logging.Level.INFO;

public class TaskTable {
    private static final Logger logger = Logger.getLogger(Scheduler.class.getName());
    private static final ConsoleHandler handler = new ConsoleHandler();

    static {
        logger.setLevel(INFO);
        //handler.setLevel(INFO);
        //logger.addHandler(handler);
    }

    private AmazonDynamoDBClient client;
    private String tableName;

    public TaskTable(String tableName) {
        AWSCredentials c = new ProfileCredentialsProvider("default").getCredentials();
        client = new AmazonDynamoDBClient(c);
        client.setRegion(Regions.US_WEST_2);
        this.tableName = tableName;
    }

    // code from http://docs.aws.amazon.com/amazondynamodb/latest/developerguide/AppendixSampleDataCodeJava.html
    private void waitForTableToBecomeAvailable() {
        logger.info("Waiting for " + tableName + " to become ACTIVE...");

        long startTime = System.currentTimeMillis();
        long endTime = startTime + (10 * 60 * 1000);
        while (System.currentTimeMillis() < endTime) {
            DescribeTableRequest request = new DescribeTableRequest().withTableName(tableName);
            TableDescription tableDescription = client.describeTable(request).getTable();
            String tableStatus = tableDescription.getTableStatus();
            logger.info("  - current state: " + tableStatus);
            if (tableStatus.equals(TableStatus.ACTIVE.toString()))
                return;
            try {
                Thread.sleep(1000 * 20);
            } catch (Exception e) {
            }
        }
        throw new RuntimeException("Table " + tableName + " never went active");
    }
    // end code

    public void createTable() {
        if (Tables.doesTableExist(client, tableName)) {
            logger.info("Table " + tableName + " is already ACTIVE");
        } else {
            // Create a table with a primary hash key named 'Id', which holds a int
            CreateTableRequest createTableRequest = new CreateTableRequest()
                    .withTableName(tableName)
                    .withKeySchema(new KeySchemaElement().withAttributeName("Id").withKeyType(KeyType.HASH))
                    .withAttributeDefinitions(new AttributeDefinition().withAttributeName("Id").withAttributeType(ScalarAttributeType.N))
                    .withProvisionedThroughput(new ProvisionedThroughput().withReadCapacityUnits(25L).withWriteCapacityUnits(25L));
            client.createTable(createTableRequest);
            waitForTableToBecomeAvailable();
        }
    }

    public String getTableName() {
        return tableName;
    }

    public boolean setTask(int taskId, int val) {
        try {
            Map<String, AttributeValue> item = new HashMap<String, AttributeValue>();
            item.put("Id", new AttributeValue().withN(Integer.toString(taskId)));
            item.put("task_done", new AttributeValue().withN(Integer.toString(val)));

            client.putItem(tableName, item);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean updateTask(int taskId, int val) {
        try {
            Map<String, AttributeValue> key = new HashMap<String, AttributeValue>();
            key.put("Id", new AttributeValue().withN(Integer.toString(taskId)));

            Map<String, AttributeValue> expressionAttributeValues = new HashMap<String, AttributeValue>();
            expressionAttributeValues.put(":val1", new AttributeValue().withN(Integer.toString(val)));
            expressionAttributeValues.put(":val2", new AttributeValue().withN("0"));

            UpdateItemRequest updateItemRequest = new UpdateItemRequest()
                    .withTableName(tableName)
                    .withKey(key)
                    .withUpdateExpression("set task_done = :val1")
                    .withConditionExpression("task_done = :val2")
                    .withExpressionAttributeValues(expressionAttributeValues);

            try {
                UpdateItemResult updateItemResult = client.updateItem(updateItemRequest);
            } catch (ConditionalCheckFailedException conde) {
                logger.info("Condition " + updateItemRequest.getConditionExpression() + " failed for task #" + taskId + ".");
                return false;
            }
            logger.info("Update of task #" + taskId + " in DynamoDB OK.");
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean isCompleted(int taskId) {
        try {
            HashMap<String, AttributeValue> key = new HashMap<String, AttributeValue>();
            key.put("Id", new AttributeValue().withN(Integer.toString(taskId)));

            GetItemResult result = client.getItem(tableName, key);
            Map<String, AttributeValue> map = result.getItem();

            if ("1".equals(map.get("task_done").getN())) {
                return true;
            } else {
                return false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return true;
        }
    }
}
