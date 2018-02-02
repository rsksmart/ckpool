
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

processResults();

function processResults() {

    var artifactName = artifacts.ckpool;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    console.log('> Processing results for {0} -- Server side'.format(artifactName));

    var artifactLocation = path.join(config.deploy.directory, artifactName);
    var ckpoolLogLocation = "../../run/logs/ckpool.log";

    async().exec(function (next) {

        var cmd = "cd ~/{0}/ && ".format(path.join(artifactLocation, "metrics", "results"));
        cmd += "rm -f actions.csv && rm -f summary.csv && ";
        cmd += "python3 ../tools/logparser.py {0} -s -o actions.csv >> summary.csv".format(ckpoolLogLocation);

        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .then(function (data, next) {

        var outputFileName = "../../metrics/results/miningMetrics.csv";
        var cmd = "cd ~/{0}/ && ".format(path.join(artifactLocation, "run", config.ckpool.logDir));

        cmd += "echo 'shares, blocks submitted to BTC, blocks submitted to RSK, getBlockTemplate' > {0} && ".format(outputFileName);

        cmd += 'echo -n "$(grep "mining.submit" ckpool.log | wc -l)," >> {1} && '.format(ckpoolLogLocation, outputFileName);
        cmd += 'echo -n "$(grep "] BLOCK ACCEPTED" {0} | wc -l)," >> {1} && '.format(ckpoolLogLocation, outputFileName);
        cmd += 'echo -n "$(grep ": submitBitcoinSolution:" ckpool.log | wc -l)," >> {1} && '.format(ckpoolLogLocation, outputFileName);
        cmd += 'echo "$(grep ": getblocktemplate:" {0} | wc -l)" >> {1}'.format(ckpoolLogLocation, outputFileName);

        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .error(function (err) {
        console.log(err);
    });
}


