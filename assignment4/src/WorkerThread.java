import java.util.concurrent.BlockingQueue;
import java.util.logging.ConsoleHandler;
import java.util.logging.Logger;

import static java.util.logging.Level.ALL;
import static java.util.logging.Level.INFO;

public class WorkerThread extends Thread {
    private static final Logger logger = Logger.getLogger(WorkerThread.class.getName());
    private static final ConsoleHandler handler = new ConsoleHandler();

    static {
        logger.setLevel(ALL);
        handler.setLevel(INFO);
        logger.addHandler(handler);
    }

    private BlockingQueue tasks = null;
    private BlockingQueue results = null;
    private boolean shutdown = false;

    public WorkerThread(BlockingQueue tasks, BlockingQueue results) {
        this.tasks = tasks;
        this.results = results;
    }

    @Override
    public void run() {
        while (!shutdown) {
            try {
                Task t = (Task) tasks.take();
                if (t.getId() == -1) { // kill pill
                    this.shutdown();
                    break;
                }
                logger.info("Running task: " + t);
                t.run();
                results.add(t);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public synchronized void shutdown() {
        shutdown = true;
        interrupt();
    }

    public synchronized boolean isRunning() {
        return !shutdown;
    }
}
