import java.io.*;
import java.net.Socket;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.ConsoleHandler;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static java.util.logging.Level.ALL;

public class Client {
    private static final Logger logger = Logger.getLogger(Client.class.getName());
    private static final ConsoleHandler handler = new ConsoleHandler();
    static {
        logger.setLevel(ALL);
    }
    private static long t_begin;
    private static long t_end;

    /**
     * Print the usage of the client CLI
     */
    private static void printUsage() {
        logger.severe("Usage :\n" +
                "client -s <IP_ADDRESS:PORT> -w <WORKLOAD_FILE>");
        System.exit(1);
    }

    /**
     * Parse the workload file and return an iterable list of tasks
     * @param workloadFile path to the file containing the workload
     * @return list of Task objects
     */
    private static List<Task> loadTasksFromFile(String workloadFile) {
        BufferedReader br = null;
        List<Task> tasks = new LinkedList<Task>();

        try {
            String line;
            String[] args;
            br = new BufferedReader(new FileReader(workloadFile));
            int i = 0;

            while ((line = br.readLine()) != null) {
                args = line.split(" ");
                Task task = new Task(i, args);
                tasks.add(task);
                i++;
            }
            return tasks;
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (br != null)
                    br.close();
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
        return null;
    }

    /**
     * Send the tasks to the scheduler
     * @param tasks list of tasks to be processed
     * @param reader stream reader to receive messages from the scheduler
     * @param writer stream writer to send messages to the scheduler
     * @throws IOException
     */
    private static void taskToScheduler(List<Task> tasks, BufferedReader reader, PrintWriter writer) throws IOException {
        logger.info("Tasks count: " + tasks.size());
        t_begin = System.currentTimeMillis();
        int count = 0;
        for (Task t : tasks) {
            writer.println(t);
            count++;
            if (count % 500 == 0) {
                logger.info("Sent " + count + " tasks to the scheduler.");
            }
        }
        writer.println("done");
        String scheduler_ack = null;
        while (null == (scheduler_ack = reader.readLine())) ;
        logger.info("Scheduler sent " + scheduler_ack);
    }

    public static void main(String[] args) throws Exception {
        logger.info("Welcome to the Client!");
        try {
            if (!args[0].equals("-s") || !args[2].equals("-w")) {
                printUsage();
            } else {

                try {
                    String host = args[1];
                    String workloadFile = args[3];

                    logger.info("Name of file containing workload: " + workloadFile);
                    int port = 0;
                    Pattern p = Pattern.compile("^\\s*(.*?):(\\d+)\\s*$");
                    Matcher m = p.matcher(host);

                    if (m.matches()) {
                        host = m.group(1);
                        port = Integer.parseInt(m.group(2));
                        logger.info("Connecting to " + host + " on port " + port);
                    } else {
                        printUsage();
                    }
                    Socket client = new Socket(host, port);
                    while (!client.isConnected()) ;
                    logger.info("Just connected to " + client.getRemoteSocketAddress());
                    PrintWriter writerToServer = new PrintWriter(client.getOutputStream(), true);
                    BufferedReader readerFromServer = new BufferedReader(new InputStreamReader(client.getInputStream()));

                    List<Task> tasks = loadTasksFromFile(workloadFile);
                    List<Integer> results = new ArrayList<Integer>();
                    List<Integer> results_ids = new ArrayList<Integer>();

                    int resultsReceived = 0;
                    taskToScheduler(tasks, readerFromServer, writerToServer);

                    logger.fine(resultsReceived + " result(s) received and tasks size = " + tasks.size());
                    while (resultsReceived < tasks.size()) {
                        logger.fine("Polling scheduler for new results.");
                        writerToServer.println("poll");
                        String poll_return = readerFromServer.readLine();
                        logger.fine("Scheduler returned: " + poll_return);
                        if ("-1".equals(poll_return)) {
                            Thread.sleep(100);
                            continue;
                        }

                        LinkedList<Integer> tids = new LinkedList<Integer>();
                        for (String tid_str : poll_return.split(" ")) {
                            tids.add(Integer.parseInt(tid_str));
                        }
                        for (Integer tid : tids) {
                            writerToServer.println("get " + tid);
                            String[] get_return = readerFromServer.readLine().split(" ");
                            int task_id = Integer.parseInt(get_return[0]);
                            int task_result = Integer.parseInt(get_return[1]);
                            logger.fine("Task " + task_id + " result: " + task_result);
                            results.add(task_result);
                            results_ids.add(task_id);
                            resultsReceived++;
                            if (resultsReceived % 500 == 0) {
                                logger.info(resultsReceived + " results received.");
                            }
                        }
                    }
                    t_end = System.currentTimeMillis();
                    long total_time = t_end - t_begin;
                    logger.info("Process task time: " + total_time + " ms.");
                    writerToServer.close();
                    readerFromServer.close();
                    client.close();

                } catch (IOException e) {
                    e.printStackTrace();
                }

            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
