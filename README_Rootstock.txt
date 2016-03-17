
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

TODO. 

I will assume that it is up and running at 192.168.0.118:4444



Ckpool
------

1) Inside ckpool foler we create a directory to run out tests

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
		"url" : "192.168.0.118:4444",
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
[2016-03-17 16:07:07.846] Connected to rskd: 192.168.0.118:4444
```

4) It should be ready to connect with a miner


For example it can be used with this miner https://bitcointalk.org/index.php?topic=55038.0


> minerd -a sha256d -t 2 --url=stratum+tcp://192.168.0.121:3333 --userpass=user:pass

"a" : sha256d to mine in bitcoind mode
"t" : to use 2 threads
"url" : addres of ckpool
"userpass" : The user:pass will be ignored by ckpool in standalone mode.

