
var Client = require('ssh2').Client;
var path = require('path');
var config = require('./config.json');
var utils = require('./lib/utils');
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

stop();

function stop() {

    stopMinerd();
    stopRootstockJ();
    stopBitcoind();
}

function stopMinerd() {

    var artifactName = artifacts.minerd;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Stopping', artifactName);

    var artifactLocation = path.join(config.deploy.directory, artifactName);

    var cmd = "cd ~/{0}/ && ./kill.sh".format(artifactLocation);
    console.log("executing remote command", cmd);

    utils.executeRemote(machine, cmd, function (err, data) {
            if (err)
                return console.log(err.toString());
        });
}

function stopRootstockJ() {

    var artifactName = artifacts.rootstockJ;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Stopping', artifactName);

    var cmd = "tmux kill-session -t {0}".format(artifactName);
    console.log("executing remote command", cmd);

    utils.executeRemote(machine, cmd, function (err, data) {
            if (err)
                return console.log(err.toString());
        });
}

function stopBitcoind() {

    var artifactName = artifacts.bitcoind;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Stopping', artifactName);

    var cmd = "bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32591 stop && ";
    cmd += "bitcoin-cli -regtest -rpcconnect=127.0.0.1 -rpcuser=admin -rpcpassword=admin -rpcport=32592 stop";
    console.log("executing remote command", cmd);

    utils.executeRemote(machine, cmd, function (err, data) {
            if (err)
                return console.log(err.toString());
        });
}


