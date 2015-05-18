# Assignment 2

## Wordcount

### Python

The docopt Python module is required to run the Wordcount, it can be installed using the following command:
  

```
sudo pip-install docopt
```

Once docopt is installed, you can run the wordcount by going into `src/python-wordcount/` and run:

```
python wordcount.py <n_shards> <n_processes> <filepath>
```

where:

- `n_shards` is the number of shards to create from the input file
- `n_processes` is the number of parallel processes to run
- `filepath` is the path to the dataset

Note that the output is directly written in the standard output so one can redirect it in a file using `>`. For example, if one wants to run wordcount on the small dataset with 4 processes running in parallel and by splitting the file in 10 shards, one can use the following command:

```
python wordcount.py 10 4 small_dataset > python-wordcount.txt
```

In this example, the wordcount output will be written in the file `python-wordcount.txt`.

### Hadoop

The install_hadoop.sh is a bash script which automates the install of Hadoop for the master ,(for the workers one just needs to comment the last line):
```
sudo chmod +x install_hadoop.sh
./install_hadoop.sh
```

Since the adresses of nodes have to be manually configured in configuration files, one needs to modify those configuration files:
/etc/hadoop/core-site.xml
/etc/hadoop/yarn-site.xml
/etc/hadoop/mapred-site.xml 
/etc/haddoop/slaves

by adding the Master address when required or the slaves addresses in /etc/haddoop/slaves

Finally you can run:
```
 sbin/start-all.sh 
```
to start the daemons on all the nodes


### Swift

Let us remember that the whole process for the Swift wordcount is divided in two parts:

- a sequential part creates shards from the input dataset by calling `split`,
- a parallel part (written in Swift) distributes the wordcount jobs over the workers and cumulate the results into a global wordcount output.

These two parts are regrouped under one unique shell script `src/swift-wordcount/launch_wordcount.sh` that also creates the directories where the shards are stored. To run the program locally, the first line of `src/swift-wordcount/swift.conf` must be `sites: local`. To run it on EC2, the first line of the same file must be `sites: cloud-static` (default value).
To run the Swift wordcount, it is required to have Java and Swift installed, either on a local machine or on several nodes in EC2. To run the Swift wordcount, one can use the following command:

```
sh ./launch_wordcount.sh <n_shards> <filepath>
```

where:

- `n_shards` is the number of shards to create from the input file
- `filepath` is the path to the dataset

The output is stored in `wordcount-swift.txt`.
