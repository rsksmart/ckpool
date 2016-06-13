
var sargs = require('simpleargs');
var config = require('./config.json');
var fs = require('fs');
var fse = require('fs-extra');
var mkdirp = require('mkdirp');
var path = require('path');
var execSync = require('child_process').execSync;
var ajgenesis = require('ajgenesis');
var utils = require('./lib/utils');

sargs.define('u','user', null, 'User');
sargs.define('q', 'quick', false, 'use existing ckpool + notifier', { flag: true });

var argv = sargs(process.argv.slice(2));

if (!argv.user) {
    console.log("Should specify machine user using -u <username>");
    process.exit(-1);
}

var artifacts =
{
    "minerd": "minerd",
    "bitcoind": "bitcoind",
    "rootstockJ": "rootstockJ",
    "ckpool": "ckpool"
};

ajgenesis.fs.createDirectory('build');

build();

function build() {

	buildCkpool();
	buildMinerd();
	buildRootstockJ();
}

function buildCkpool() {   

    var artifactName = artifacts.ckpool;
    var machine = utils.getMachineInfo(artifactName, config);

	mkdirp.sync(path.join('build', artifactName, 'metricstools'));
	mkdirp.sync(path.join('build', artifactName, 'run', 'conf'));

    var ckpoolConfig = config.ckpool;
    var btcdMachine = utils.getMachineInfo(artifacts.bitcoind, config);
    ckpoolConfig.btcd[0].url = btcdMachine.private.ip + ":32591";
   	
   	var rskMachine = utils.getMachineInfo(artifacts.rootstockJ, config);
    ckpoolConfig.rskd[0].url = rskMachine.private.ip + ":4444";

    ckpoolConfig.serverUrl = [ machine.private.ip + ":3333" , "127.0.0.1:3333"];

    ajgenesis.fileTransform(path.join('templates', 'ckpool.conf.tpl'), path.join('build', 'ckpool', 'run', 'conf', 'ckpool.conf'), ckpoolConfig);
    
    var ckpoolVersion = config.testcases[0].mergeMining ? "ckpool_rsk" : "ckpool_original";
    if(!argv.quick) {
    	////TODO: build ckpool and notifier sync
    }

    fse.copySync(path.join("executables", artifactName, ckpoolVersion), path.join("build", artifactName, "run", ckpoolVersion));

	var	notifierModel = {
		"user": argv.user,
		"machine": machine.private.ip,
		"notifier": "./" + path.join(config.deploy.directory, artifactName, "run", "notifier")
	};
	var notifierDir = path.join('build', 'bitcoind');
	mkdirp.sync(notifierDir);
    ajgenesis.fileTransform(path.join('templates', 'notifier.sh.tpl'), path.join(notifierDir, 'notifier.sh'), notifierModel);
    fse.copySync(path.join("executables", artifactName, "notifier"), path.join("build", artifactName, "run", "notifier"));

    fse.copySync(path.join("executables", artifactName, "metricstools"), path.join("build", artifactName, "metricstools"));
}

function buildMinerd() {   

    var artifactName = artifacts.minerd;
    fse.copySync(path.join("executables", artifactName), path.join("build", artifactName));
}

function buildRootstockJ() {   

    var artifactName = artifacts.rootstockJ;

    var confDir = path.join('build', artifactName, 'conf');
	mkdirp.sync(confDir);

    ajgenesis.fileTransform(path.join('templates', 'node.conf.tpl'), path.join(confDir, 'node.conf'), {});

    fse.copySync(path.join("executables", artifactName), path.join("build", artifactName));
}
