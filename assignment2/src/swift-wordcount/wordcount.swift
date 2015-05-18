type file;
type script;

// Function to run a bash script without arguments
app (file output) run_script(script s, file input) {
  bash filename(s) filename(input) stdout=filename(output);
}

app (file output) analyze(script s, file input[]) {
  bash filename(s) filenames(input) stdout=filename(output);
}

int nshards = toInt(arg("n"));
file shards[];
foreach i in [0:nshards-1] {
  string stri;
  if (i < 10) {
    stri = strcat("0", i);
  } else {
    stri = toString(i);
  } 
  file s <single_file_mapper; file=strcat("/data/inputs/d_", stri, ".shard")>;
  shards[i] = s;
}

// = Bash scripts =
script wordcount <"wordcount.sh">;
script cumul_counts <"cumul_counts.sh">;

file counts[];
foreach i in [0:nshards-1] {
  file sh = shards[i];
  file c<single_file_mapper; file=strcat("/tmp/outputs/count_", i, ".out")>;
  c = run_script(wordcount, input=sh);
  counts[i] = c;
}

file cumul_counts_output<"wordcount-swift.txt">;
cumul_counts_output = analyze(cumul_counts, counts);
