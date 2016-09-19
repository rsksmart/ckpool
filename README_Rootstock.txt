
Building ckpool
===============

ckpool should be build on linux (or a cross compiler)

0) Install tools and dependencies

> apt-get install build-essential autoconf automake libtool git

1) Checkout source

> git clone https://github.com/rootstock/ckpool.git

2) Generate configure files
 
> ./autogen.sh

3) Generate Makefile. It is easier to setup without ckdb. ckdb is a backend that uses postgresql to store miners information. 

> ./configure --without-ckdb

4) Compilar

> make

The compiled binaries will be in ./src


Testing
=======

Bitcoind
--------

1) Create a directories for bitcoind, and two diretories one for each instance

> mkdir btcd
> cd btcd
> mkdir A
> mkdir B


2) Now we launch bitcoind in regtest mode

> bitcoind -server -listen -port=31591 -rpcuser=admin -rpcpassword=admin -rpcport=32591 -datadir=./A -connect=localhost:31592 -regtest -daemon -debug

> bitcoind -server -listen -port=31592 -rpcuser=admin -rpcpassword=admin -rpcport=32592 -datadir=./B -connect=localhost:31591 -regtest -daemon -debug


3) We now can verify that it is running

> bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32591 getinfo


4) The output have to something like this

{
  "version": 120000,
  "protocolversion": 70012,
  "walletversion": 60000,
  "balance": 0.00000000,
  "blocks": 0,
  "timeoffset": 0,
  "connections": 2,
  "proxy": "",
  "difficulty": 4.656542373906925e-10,
  "testnet": false,
  "keypoololdest": 1458240195,
  "keypoolsize": 101,
  "paytxfee": 0.00000000,
  "relayfee": 0.00001000,
  "errors": ""
}

5) Check what getblocktemplate returns

> bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32591 getblocktemplate

If it returns an error

```
error code: -10
error message:
Bitcoin is downloading blocks...
```

We generate a block to start

> bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32591 generate 1

```
[
  "716162d5acae71e322ab4531e2ac0be0771f110c4fd213c4306f7b7d09e45094"
]
```

Now getblocktemplate should work

> bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32591 getblocktemplate

The output should be something like

```
{
  "capabilities": [
    "proposal"
  ],
  "version": 4,
  "previousblockhash": "716162d5acae71e322ab4531e2ac0be0771f110c4fd213c4306f7b7d09e45094",
  "transactions": [
  ],
  "coinbaseaux": {
    "flags": ""
  },
  "coinbasevalue": 5000000000,
  "longpollid": "716162d5acae71e322ab4531e2ac0be0771f110c4fd213c4306f7b7d09e450944",
  "target": "7fffff0000000000000000000000000000000000000000000000000000000000",
  "mintime": 1458241314,
  "mutable": [
    "time",
    "transactions",
    "prevblock"
  ],
  "noncerange": "00000000ffffffff",
  "sigoplimit": 20000,
  "sizelimit": 1000000,
  "curtime": 1458241367,
  "bits": "207fffff",
  "height": 2
}
```


RootstockJ
----------

0) Install Oracle Java 8

  > sudo add-apt-repository ppa:webupd8team/java
  > sudo apt-get update
  > sudo apt-get install oracle-java8-installer

1) Checkout RootstockJ source in a directory named RootstockJ

  > git clone https://github.com/rootstock/rootstockJ.git

2) Generate fat jar for RootstockJ
 
  - Inside RootstockJ's directory (where it was cloned on step 1), run the following command:

  > gradle shadow

- The fat jar will be generated in ethereumj-core/build/libs/

3) Run miner

  > java -Dethereumj.conf.file=node1.conf -cp rootstock.jar io.rootstock.Start

  - rootstock.jar is the name of the fatjar.
  - Configuration file for node1 is named node1.conf.
  - node1.conf is in the same folder as the jar.
  - Node1 will be listening for RPC at 127.0.0.1:4444.


Ckpool
------

1) Inside ckpool folder we create a directory to run out tests

> mkdir run
> cd run

So we have the binaries in ckpool/src and our test will run on ckpool/run


2) We need to create a configuration file ckpool.conf with the following content

```
{
"btcd" :  [
	{
		"url" : "127.0.0.1:32591",
		"auth" : "admin",
		"pass" : "admin",
		"notify" : false
	}
],
"rskd" :  [
	{
		"url" : "127.0.0.1:4444",
		"auth" : "user",
		"pass" : "pass"
	}
],
"upstream" : "main.ckpool.org:3336",
"btcaddress" : "n19XNjTfYwLx3YbQk7GG2tLjzcqUeWP42D",
"btcsig" : "/mined by ck/",
"blockpoll" : 10000,
"nonce1length" : 4,
"nonce2length" : 8,
"update_interval" : 30,
"serverurl" : [
	"192.168.0.121:3333",
        "127.0.0.1:3333"
],
"nodeserver" : [
	"127.0.0.1:3335"
],
"trusted" : [
	"127.0.0.1:3336"
],
"mindiff" : 0,
"startdiff" : 1,
"maxdiff" : 0,
"logdir" : "logs",
"rskpollperiod" : 10000,
"rsknotifypolicy": 0
}
```

"btcd" : entry should point to the bitcoind running in regtest mode.

"rskd" : should point to the rootstockj running.

"serverurl" : are the address that ckpool will be listening for miners


3) Now within run/ we can lauch ckpool

> ../src/ckpool -A -l 7 -c ckpool.conf

Flags used:

"A" : standalone mode, will accept from any miner (user:pass will be ignored)
"l" : set log level to debug (all the info is logged in run/ckpool.log) 
"c" : configuration file

It should output something like this

```
[2016-03-17 16:07:07.582] ckpool stratifier starting
[2016-03-17 16:07:07.582] ckpool connector starting
[2016-03-17 16:07:07.585] ckpool connector ready
[2016-03-17 16:07:07.587] ckpool generator starting
[2016-03-17 16:07:07.586] ckpool rootstock starting
[2016-03-17 16:07:07.604] ckpool generator ready
[2016-03-17 16:07:07.606] Connected to bitcoind: 127.0.0.1:32591
[2016-03-17 16:07:07.613] ckpool stratifier ready
[2016-03-17 16:07:07.841] ckpool rootstock ready
[2016-03-17 16:07:07.846] Connected to rskd: 127.0.0.1:4444
```

4) It should be ready to connect with a miner


For example it can be used with this miner https://bitcointalk.org/index.php?topic=55038.0


> minerd -a sha256d -t 2 --url=stratum+tcp://192.168.0.121:3333 --userpass=user:pass

"a" : sha256d to mine in bitcoind mode
"t" : to use 2 threads
"url" : addres of ckpool
"userpass" : The user:pass will be ignored by ckpool in standalone mode.


Ckpool Dev Mode
---------------

Created to allow miners to receive a very small difficulty value for mining. 
As a consequence, miners will mine at a faster rate.
Needed because on a regular computer mining can take too long.

To activate Dev Mode the value of te following line (located on file. stratifier.c) should be true:
  
  > #define DEV_MODE_ON true

false will make CKPool work as usual and all the Dev Mode settings will be ignored.

The following settings can be modified and will impact on the mining rate: 

- Miner Difficulty. Value that will be send to the miners 

  > #define MINER_DIFF  0.005

- Block submission difficulty for RSK. Value that determines when a block generated by the miner will be send to the RSK Node

  > #define RSK_CKPOOL_DIFF 0.0

- Block submission difficulty for BTC. Value that determines when a block generated by the miner will be send to Bitcoind

  > #define BTC_CKPOOL_DIFF 0.1

For example, the previous configuration allows a rate of 1 BTC Block/Min and 1 RSK Block/10 Secs 



Parsing ckpool logs
===================

Prerequisites
-------------

Install parse ( https://pypi.python.org/pypi/parse )

> pip install parse


Execution
---------

Calling it without arguments will display a usage text

> ./logparser.py

usage: logparser.py [-h] [-o OUTPUT] [-s] logFile                  
logparser.py: error: the following arguments are required: logFile 

With -o OUTPUT will extract the logged actions from the log file and will
write it to in CSV format to the file specified.

> ./logparser.py logs/ckpool.log -o actions.csv

The fields logged are: action name, start time, duration, id

> ./logparser.py logs/ckpool.log -s

With the -s flag will display a summary of the high level operations

The fields logged are: action name, start time, duration, jobid, clients, notification duration


Output example
--------------

An example of the output file is the following

mining.submit, 2016-04-12 13:40:33.718000, 0.0, 570d22f50000000f:1aeb3906
mining.submit, 2016-04-12 13:40:34.776000, 0.0, 570d22f50000000f:f1bd6587
mining.submit, 2016-04-12 13:40:36.208000, 0.0, 570d22f50000000f:10d23b88
blocksolve, 2016-04-12 13:40:36.209000, 0.0, 570d22f50000000f:10d23b88:98853eac43b833be80da52df1dfd1cb7b26f76bbe0d3b7e20779f19300000000
mining.submit, 2016-04-12 13:40:36.913000, 0.0, 570d22f50000000f:ba85a588
submitblock, 2016-04-12 13:40:36.209000, 1155.0, 98853eac43b833be80da52df1dfd1cb7b26f76bbe0d3b7e20779f19300000000
getblocktemplate, 2016-04-12 13:40:37.365000, 1003.0, 570d22f500000010
mining.notify, 2016-04-12 13:40:38.368000, 0.0, 570d22f500000010
mining.submit, 2016-04-12 13:40:39.338000, 0.0, 570d22f500000010:e7889180


Example of summary generated
----------------------------

getblocktemplate, 2016-04-12 13:38:34.106000, 1002.0, 570d22f50000000c, 1, 1.0
getblocktemplate, 2016-04-12 13:39:09.108000, 1005.0, 570d22f50000000d, 1, 1.0
getblocktemplate, 2016-04-12 13:39:44.113000, 1136.0, 570d22f50000000e, 1, 1.0
submitblock, 2016-04-12 13:40:36.208000, 1.0, 570d22f50000000f, 1, 1155.0
getblocktemplate, 2016-04-12 13:40:19.116000, 1009.0, 570d22f50000000f, 1, 1.0
getblocktemplate, 2016-04-12 13:40:37.365000, 1003.0, 570d22f500000010, 1, 0.0
submitblock, 2016-04-12 13:41:11.451000, 1.0, 570d22f500000011, 1, 1012.0
getblocktemplate, 2016-04-12 13:40:57.368000, 1046.0, 570d22f500000011, 1, 0.0
getblocktemplate, 2016-04-12 13:41:12.464000, 1006.0, 570d22f500000012, 1, 0.0
getblocktemplate, 2016-04-12 13:41:32.467000, 1007.0, 570d22f500000013, 1, 0.0



Analysis
--------

For now there are two operations being logged: getblocktemplate and submitblock.

* getblocktemplate

We interpret the following line:

getblocktemplate, 2016-04-12 13:38:34.106000, 1002.0, 570d22f50000000c, 1, 1.0

action: getblocktemplate
start: 2016-04-12 13:38:34.106000
	The time that the getblocktemplate call was send to bitcoind
duration: 1002.0
	The duration in miliseconds of the getblocktemplate
jobid: 570d22f50000000c
	The jobid assigned by ckpool
clients: 1
	The numbers of clients who received the mining.notify for the jobid
elapsed: 1.0
	The time elapsed until the last notification, in miliseconds

* submitblock

This line can be interpreted as

submitblock, 2016-04-12 13:40:36.208000, 1.0, 570d22f50000000f, 1, 1155.0

action: submitblock
start: 2016-04-12 13:40:36.208000
	The time the mining.submit was received by CKPool
duration: 1.0
	The time elapsed until the block is sent to bitcoind, in miliseconds
jobid: 570d22f50000000f
	The jobid being submitted
clients: 1
	This is always 1 for submitblock
elapsed: 1155.0
	The duration in miliseconds of the submitblock call to bitcoind
