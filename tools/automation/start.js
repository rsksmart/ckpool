
var Client = require('ssh2').Client;
var path = require('path');
var config = require('./config.json');
var utils = require('./lib/utils');
var async = require('simpleasync');
require("./lib/string.format.js");
var sargs = require('simpleargs');

sargs.define('u','user', null, 'User');
sargs.define('p','passphrase', null, 'Passphrase');

var argv = sargs(process.argv.slice(2));

if (!argv.user) {
    console.log("Should specify machine user using -u <username>");
    process.exit(-1);
}

if (!argv.passphrase) {
    console.log("Should specify SSH private key passphrase using -p <passphrase>");
    process.exit(-1);
}

var artifacts =
{
    "minerd": "minerd",
    "bitcoind": "bitcoind",
    "rootstockJ": "rootstockJ",
    "ckpool": "ckpool"
};

var testCases = config.testcases;

testCases.forEach(function(testCase) {
    start(testCase);
});

function start(testCase) {

    startMinerd(testCase);
    startRootstockJ(testCase);
    startBitcoind(testCase);
    
    var bitcoindStartTimeMs = 40 * 1000;

    setTimeout(function(){ startCkpool(testCase); }, bitcoindStartTimeMs);

    var testDurationMs = testCase.duration * 60 * 1000 + bitcoindStartTimeMs;
    setTimeout(function(){ stopCkpool(testCase); }, testDurationMs);
}

function startMinerd(testCase) {

    var artifactName = artifacts.minerd;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Starting', artifactName);

    var artifactLocation = path.join(config.deploy.directory, artifactName);

    var cmd = "cd ~/{0}/ && ".format(artifactLocation);
    for(i = 0; i < testCase.miners; i++) {
        cmd += "tmux new -d -s miner-{0} './minerd -a sha256d -t 8 --url=stratum+tcp://192.168.200.139:3333 --userpass=user:pass' && ".format(i+1);
    };

    cmd += "date";

    console.log("executing remote command", cmd);

    utils.executeRemote(machine, cmd, function (err, data) {
            if (err)
                return console.log(err.toString());
        });
}

function startRootstockJ(testCase) {

    var artifactName = artifacts.rootstockJ;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Starting', artifactName);

    var artifactLocation = path.join(config.deploy.directory, artifactName);

    var cmd = "cd ~/{0}/ && ".format(artifactLocation);
    cmd += "tmux new -d -s {0} 'java -Dethereumj.conf.file=./conf/node.conf -cp ./rootstock.jar io.rootstock.Start'".format(artifactName);

    console.log("executing remote command", cmd);

    utils.executeRemote(machine, cmd, function (err, data) {
            if (err)
                return console.log(err.toString());
        });
}

function startBitcoind(testCase) {

    var artifactName = artifacts.bitcoind;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Starting', artifactName);

    var artifactLocation = path.join(config.deploy.directory, artifactName);

    var cmd = "cd ~/{0}/ && ".format(artifactLocation);
    cmd += "rm -f -r ./A/regtest/ && ";
    cmd += "rm -f -r ./B/regtest/ && ";
    cmd += "bitcoind -server -listen -port=31591 -rpcuser=admin -rpcpassword=admin -rpcport=32591 -rpcallowip=1.2.3.4/0 -datadir=./A -connect=localhost:31592 -regtest -daemon -blocknotify=./notifier.sh -debug && "
    cmd += "bitcoind -server -listen -port=31592 -rpcuser=admin -rpcpassword=admin -rpcport=32592 -rpcallowip=1.2.3.4/0 -datadir=./B -connect=localhost:31591 -regtest -daemon -blocknotify=./notifier.sh -debug && "
    cmd += "sleep 30 && " 
    cmd += "bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32591 generate 1";

    console.log("executing remote command", cmd);

    utils.executeRemote(machine, cmd, function (err, data) {
            if (err)
                return console.log(err.toString());
        });
}

function startCkpool(testCase) {

    var artifactName = artifacts.ckpool;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Starting', artifactName);

    var artifactLocation = path.join(config.deploy.directory, artifactName);

    async().exec(function (next) {

        var ckpoolVersion = config.testcases.mergeMining ? "ckpool_rsk" : "ckpool_original";

        var cmd = "cd ~/{0}/ && ".format(path.join(artifactLocation, "run"));
        cmd += "rm -f -r {0} && ".format(config.ckpool.logDir);
        cmd += "tmux new -d -s ckp './{0} -A -l 7 -c conf/ckpool.conf'".format(ckpoolVersion);

        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .then(function (data, next) {

        var cmd = "cd ~/{0}/ && ".format(path.join(artifactLocation, "metrics", "tools"));
        cmd += "tmux new -d -s ckp-monitor './ckp-monitor-to-csv.sh > ../results/monitor.csv'";

        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .error(function (err) {
        console.log(err);
    });
}

function stopCkpool(testCase) {

    var artifactName = artifacts.ckpool;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Stopping', artifactName);

    async().exec(function (next) {

        var cmd = "tmux kill-session -t ckp";
        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .then(function (data, next) {

        var cmd = "tmux kill-session -t ckp-monitor";
        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .error(function (err) {
        console.log(err);
    });

}


