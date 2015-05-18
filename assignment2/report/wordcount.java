
        
public class WordCount {
        
 public static class Map extends Mapper<LongWritable, Text, Text, IntWritable> {
    private final static IntWritable one = new IntWritable(1);
    private Text w = new Text();
    private St
    public void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
        String line = value.toString();
        StringTokenizer tokenizer = new StringTokenizer(line);
        //String[] words = line.split("\\s+");
        //for (String w: words) {
			//wt.set(w);
           // context.write(wt, one);
        while (tokenizer.hasMoreTokens()) {
            w.set(tokenizer.nextToken());
            context.write(w, one);
        }
    }
 } 
        
 
