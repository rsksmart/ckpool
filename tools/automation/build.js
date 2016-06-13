
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
    ajgenesis.fileTransform(path.join('templates', 'ckpool.conf.tpl'), path.join('build', 'ckpool', 'run', 'conf', 'ckpool.conf'), ckpoolConfig);

	var	notifierModel = {
		"user": argv.user,
		"machine": machine.private.ip,
		"notifier": "./" + path.join(config.deploy.directory, artifactName, "run", "notifier")
	};
	var notifierDir = path.join('build', 'bitcoind');
	if (!fs.existsSync(notifierDir)){
    	fs.mkdirSync(notifierDir);
    }
    ajgenesis.fileTransform(path.join('templates', 'notifier.sh.tpl'), path.join(notifierDir, 'notifier.sh'), notifierModel);

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
