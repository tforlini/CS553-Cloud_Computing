import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;

public class ThreadPool {
    private BlockingQueue tasks;
    private BlockingQueue results;
    private int nthreads;
    private List<WorkerThread> threads;
    private boolean running;

    public ThreadPool(int nthreads, BlockingQueue tasks, BlockingQueue results) {
        this.running = true;
        this.nthreads = nthreads;
        this.tasks = tasks;
        this.results = results;
        threads = new ArrayList<WorkerThread>(nthreads);
        for (int i = 0; i < nthreads; i++) {
            threads.add(new WorkerThread(tasks, results));
            tasks.add(new Task(-1, null)); // kill pill
        }
        for (WorkerThread th : threads) {
            th.start();
        }
    }

    public synchronized void stop() {
        running = false;
        for (WorkerThread th : threads) {
            th.shutdown();
        }
    }

    private boolean isRunning() {
        return running;
    }

    public BlockingQueue getResults() {
        return results;
    }
}
