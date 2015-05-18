import java.util.Arrays;

public class Task implements Runnable {
    private int id;
    private String[] args;
    private boolean completed;
    private int result;

    /**
     * Initialize the id and args value with given values
     * @param id id value to initialize
     * @param args args value to initialize
     */
    private void init(int id, String[] args) {
        this.id = id;
        this.args = args;
    }

    public Task(int id, String[] args) {
        init(id, args);
        completed = false;
        result = -1;
    }

    /**
     * Parse a message and build a Task from it.
     * @param str the message that contains the task
     */
    public Task(String str) {
        String split[] = str.split(" ");
        int id = Integer.parseInt(split[0]);
        String[] args = Arrays.copyOfRange(split, 1, split.length);
        init(id, args);
    }

    /**
     * Default constructor
     */
    public Task() {
        init(0, null);
        completed = false;
        result = -1;
    }

    public void setResult(int result) {
        this.result = result;
    }

    public void setId(int id) {
        this.id = id;
    }

    public void setArgs(String[] args) {
        this.args = args;
    }

    public int getId() {
        return id;
    }

    public String[] getArgs() {
        return args;
    }

    public boolean isCompleted() {
        return completed;
    }

    public int getResult() {
        return result;
    }

    @Override
    public String toString() {
        if (id == -1) {
            return "KILLPILL";
        }
        StringBuilder sb = new StringBuilder(Integer.toString(id));
        for (String arg : args) {
            sb.append(" " + arg);
        }
        return sb.toString();
    }

    @Override
    public void run() {
        try {
            if ("sleep".equals(args[0])) {
                Thread.sleep(Integer.parseInt(args[1]) * 1000);
                result = 0;
            } else {
                result = 1;
            }
        } catch (Exception e) {
            result = 1;
        } finally {
            completed = true;
        }
    }
}
