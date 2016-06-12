
var sargs = require('simpleargs');
var config = require('./config.json');
var path = require('path');
var utils = require('./lib/utils');
var async = require('simpleasync');
var fs = require('fs');

sargs.define('u','user', null, 'User');
sargs.define('p','passphrase', null, 'Passphrase');
sargs.define('q', 'quick', false, 'only copy configuration files', { flag: true });

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

deploy();

function deploy() {
    
    deployMinerd();
    deployBitcoind();
    deployRootstockJ();
    deployCkpool();
    deployMetricsTools();
}

function deployMinerd() {

    var artifactName = artifacts.minerd;
    console.log('> Deploying artifact', artifactName);  

    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    var artifactDirectory = path.join(config.deploy.directory, artifactName);

    async().exec(function (next) {

        utils.executeRemote(machine, "mkdir -p " + artifactDirectory, next);
    })
    .then(function (data, next) {

        if (argv.quick)
            return next(null, null);

        var host = machine.public.ip || machine.private.ip;
        
        var directory = path.join('build', artifactName);

        var destination = artifactDirectory;
        
        utils.copyRemote(machine, directory, destination, next);
    })
    .then(function (data, next) {

        if (argv.quick)
            return next(null, null);

        var cmd = "chmod +x ./" + artifactDirectory + "/" + artifactName;

        console.log("executing remote comand", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .error(function (err) {
        console.log(err);
    });
}

function deployBitcoind() {

    var artifactName = artifacts.bitcoind;
    console.log('> Deploying artifact', artifactName);

    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    async().exec(function (next) {

        var directory = path.join(config.deploy.directory, artifactName, "A");

        utils.executeRemote(machine, "mkdir -p " + directory, next);
    })
    .then(function (data, next) {

        var directory = path.join(config.deploy.directory, artifactName, "B");

        utils.executeRemote(machine, "mkdir -p " + directory, next);
    })
    .error(function (err) {
        console.log(err);
    });
}

function deployRootstockJ() {

    var artifactName = artifacts.rootstockJ;
    console.log('> Deploying artifact', artifactName);

    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    var artifactDirectory = path.join(config.deploy.directory, artifactName);

    async().exec(function (next) {

        utils.executeRemote(machine, "mkdir -p " + artifactDirectory, next);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join('build', artifactName);
        var destination = artifactDirectory;

        if (argv.quick) {
            source = path.join('build', artifactName, "conf");
            destination = path.join(artifactDirectory, "conf");
        }
        
        utils.copyRemote(machine, source, destination, next);
    })
    .error(function (err) {
        console.log(err);
    });
}

function deployCkpool() {

    var artifactName = artifacts.ckpool;
    console.log('> Deploying artifact', artifactName);

    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    var artifactDirectory = path.join(config.deploy.directory, artifactName, "run");
    
    async().exec(function (next) {

        utils.executeRemote(machine, "mkdir -p " + artifactDirectory, next);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join('build', artifactName, "run");
        var destination = artifactDirectory;

        if (argv.quick) {
            source = path.join('build', artifactName, "run", "conf");
            destination = path.join(artifactDirectory, "conf");
        }

        utils.copyRemote(machine, source, destination, next);
    })
    .then(function (data, next) {

        if (argv.quick)
            return next(null, null);

        var cmd = "chmod +x ./" + artifactDirectory + "/" + artifactName + "_original";
        console.log("executing remote comand", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .then(function (data, next) {

        if (argv.quick)
            return next(null, null);

        var cmd = "chmod +x ./" + artifactDirectory + "/" + "notifier";
        console.log("executing remote comand", cmd);

        utils.executeRemote(machine, cmd, next);
    })
    .error(function (err) {
        console.log(err);
    });

    deployNotifierForBitcoind(argv, machine);
}

function deployNotifierForBitcoind(argv, machine) {
    
    if (argv.quick)
        return;

    var artifactName = artifacts.bitcoind;
    console.log("> Deploying artifact notifier");

    var notifierMachine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(notifierMachine, argv, config);

    var artifactDirectory = path.join(config.deploy.directory, artifactName);

    async().exec(function (next) {

        var host = machine.public.ip || machine.private.ip;
        
        var directory = path.join("build", artifactName);
        var destination = artifactDirectory;
        
        utils.copyRemote(notifierMachine, directory, destination, next);
    })
    .then(function (data, next) {

        var cmd = "chmod +x ./" + artifactDirectory + "/notifier.sh";
        console.log("executing remote comand", cmd);

        utils.executeRemote(notifierMachine, cmd, next);
    })
    .error(function (err) {
        console.log(err);
    });
}

function deployMetricsTools() {

    if (argv.quick)
        return;

    console.log("> Deploying metric's tools");

    var artifactName = artifacts.ckpool;
    var machine = utils.getMachineInfo(artifactName, config);
    utils.addSshConnectionInfo(machine, argv, config);

    var artifactDirectory = path.join(config.deploy.directory, artifactName, "metrics");

    async().exec(function (next) {

        utils.executeRemote(machine, "mkdir -p " + path.join(artifactDirectory, "tools"), next);
    })
    .then(function (data, next) {

        utils.executeRemote(machine, "mkdir -p " + path.join(artifactDirectory, "results"), next);
    })
    .then(function (data, next) {

        var host = machine.public.ip || machine.private.ip;
        
        var source = path.join("build", artifactName, "metricstools");
        var destination = path.join(artifactDirectory, "tools");
        
        utils.copyRemote(machine, source, destination, next);
    })
    .error(function (err) {
        console.log(err);
    });
}

