
var Client = require('ssh2').Client;
var path = require('path');
var config = require('./config.json');
var utils = require('./lib/utils');
var async = require('simpleasync');
require("./lib/string.format.js");
var sargs = require('simpleargs');
var fs = require('fs');
var stats = require("stats-lite")

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

    console.log('> Processing results for {0} -- Client side'.format(artifactName));

    var sourceDirectory = path.join(config.deploy.directory, artifactName, "metrics", "results");
    var resultsDirectory = path.join("results") + "/";

    async().exec(function (next) {

        if(!fs.existsSync(resultsDirectory)) {
            fs.mkdirSync(resultsDirectory);
        }

        next(null, null);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join(sourceDirectory, "actions.csv");
        var destination = resultsDirectory;

        utils.downloadRemote(machine, source, destination, next);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join(sourceDirectory, "summary.csv");
        var destination = resultsDirectory;

        utils.downloadRemote(machine, source, destination, next);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join(sourceDirectory, "miningMetrics.csv");
        var destination = resultsDirectory;

        utils.downloadRemote(machine, source, destination, next);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join(sourceDirectory, "monitor.csv");
        var destination = resultsDirectory;

        utils.downloadRemote(machine, source, destination, next);
    })
    .then(function (data, next) {

        var submitBlock = [];
        var getblocktemplate = [];

        var summaryLines = fs.readFileSync('results/summary.csv').toString().split('\n').forEach(function(line) {
            
            if(line.startsWith("getblocktemplate")) {

                var time = line.split(",")[5];

                if(!isNaN(time)) {
                    //console.log("getblocktemplate time:", time);
                    getblocktemplate.push(time);
                }
            } 

            if(line.startsWith("submitblock")) {

                var time = line.split(",")[2];

                if(!isNaN(time)) {
                    //console.log("submitblock time:", splittedSubmit[2]);
                    submitBlock.push(time);
                }
            }
        });

        // console.log("notif pool -> miner | average", stats.mean(getblocktemplate));
        // console.log("notif pool -> miner | SD", stats.stdev(getblocktemplate));
        // console.log("notif pool -> miner | max", Math.max.apply(Math, getblocktemplate));
        // console.log("notif pool -> miner | median", stats.median(getblocktemplate));
        // console.log("notif pool -> miner | min", Math.min.apply(Math, getblocktemplate));

        // console.log("submit miner -> bitcoin | average", stats.mean(submitBlock));
        // console.log("submit miner -> bitcoin | SD", stats.stdev(submitBlock));
        // console.log("submit miner -> bitcoin | max", Math.max.apply(Math, submitBlock));
        // console.log("submit miner -> bitcoin | median", stats.median(submitBlock));
        // console.log("submit miner -> bitcoin | min", Math.min.apply(Math, submitBlock));

        var miningMetricsLines = fs.readFileSync('results/miningMetrics.csv').toString().split('\n');
        // console.log(miningMetricsLines[0]);
        // console.log(miningMetricsLines[1]);

        var monitorLines = fs.readFileSync('results/monitor.csv').toString().split('\n');
        // console.log(monitorLines[0]);
        // console.log(monitorLines[monitorLines.length-2]);

        var stats = buildStatsHeader(miningMetricsLines, monitorLines) + "\n";
        stats += getStats(getblocktemplate, submitBlock, miningMetricsLines, monitorLines);

        fs.writeFileSync(resultsDirectory + "stats.csv", stats);
        console.log("Stats calculation finished !!");

        next(null, null);
    })
    .then(function (data, next) {

        process.exit();
    })
    .error(function (err) {
        console.log(err);
    });
}

function buildStatsHeader(miningMetricsLines, monitorLines) {

    var statsHeader = "notif pool -> miner | average,";
    statsHeader += "notif pool -> miner | SD,";
    statsHeader += "notif pool -> miner | max,";
    statsHeader += "notif pool -> miner | median,";
    statsHeader += "notif pool -> miner | min,";
    statsHeader += "submit miner -> bitcoin | average,";
    statsHeader += "submit miner -> bitcoin | SD,";
    statsHeader += "submit miner -> bitcoin | max,";
    statsHeader += "submit miner -> bitcoin | median,";
    statsHeader += "submit miner -> bitcoin | min,";
    statsHeader += miningMetricsLines[0] + ","; 
    statsHeader += monitorLines[0].substring(monitorLines[0].indexOf(",") + 1); 

    return statsHeader;
}

function getStats(getblocktemplate, submitBlock, miningMetricsLines, monitorLines) {

    var statsForFile = stats.mean(getblocktemplate) + ",";
    statsForFile += stats.stdev(getblocktemplate) + ",";
    statsForFile += Math.max.apply(Math, getblocktemplate) + ",";
    statsForFile += stats.median(getblocktemplate) + ",";
    statsForFile += Math.min.apply(Math, getblocktemplate) + ",";

    statsForFile += stats.mean(submitBlock) + ",";
    statsForFile += stats.stdev(submitBlock) + ",";
    statsForFile += Math.max.apply(Math, submitBlock) + ",";
    statsForFile += stats.median(submitBlock) + ",";
    statsForFile += Math.min.apply(Math, submitBlock) + ",";

    statsForFile += miningMetricsLines[1] + ","; 
    var lastElem = monitorLines[monitorLines.length-2];
    statsForFile += lastElem.substring(lastElem.indexOf(",") + 1); 

    return statsForFile;
}

