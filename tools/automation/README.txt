Test automation framework for CKpool
------------------------------------
About
-----

The automation framework for CKPool is an OS agnostic set of scripts written in node js. Each of them has a unique responsability. All toghether, they allow to run a ckpool test case from scratch and obtain the correspinding stats from its result.

The available scripts are:

- build. Generates all the needed files for a ckpool test case (i.e. executables, bash scripts, configuration files)
- deploy. Copies all the files to the machines were they will be executed.
- start. Runs a test case.
- stop. Stops all the components needed to run a test case (i.e. bitcoind, miners)
- processServerSide. Generates all the statistics based on the test results.
- processClientSide. Downloads and merges the statistics already generated on the server. "stats.csv" has the final results.
- clean. Deletes all the files from the machines used ti run the tests.

Requirements
------------
The installation requirements for the following programs must be fulfilled in the machines were they will be run:
- Minerd
- Bitcoind 
- RootstockJ

A parser called "logparser.py" is also used on the machines involved in the test run and its requirements are:
> sudo apt install python3-pip
> pip3 install parse

SSH connection using public/private key must be configured in all the machines that will run the tests. The pair of keys should be generated with the empty password. 

The machine launching the test should have node js and all the packages listed in the file "package.json" installed.
It also needs an SSH connection to each of the machines that will be executing the test (using public/private key configuration).

How to run
----------

-- The expected flow for running a test case is:

	build --> deploy --> start --> stop --> processServerSide --> processClientSide --> clean

	note. clean may not be necessary to execute if many tests are going to be run.

-- To run a node script use on the command line:

	> node build.js -u rootstock

-- There is a master configuration file for setting up all the related options of a test. It is config.json

-- Some scripts require arguments like the remote machine user and password for SSH which can be set via command line arguments.

-- Quick Mode (a.k.a "-q" flag)
	- In build.js it avoids building ckpool and uses a prebuilt version located on the executables directory.
	- In deploy.js it only copies the configuration files and not the executables to the machines that will be running the test.

