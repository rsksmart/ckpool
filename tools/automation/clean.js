
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

clean();

function clean() {

    for(i = 0; i < config.machines.length; i++) {

        var machine = config.machines[i];
        utils.addSshConnectionInfo(machine, argv, config);

        console.log('> Cleaning', machine.name);

        var cmd = "rm -f -r " + config.deploy.directory;
        console.log("executing remote command", cmd);

        utils.executeRemote(machine, cmd, function (err, data) {
                if (err)
                    return console.log(err.toString());
            });

    }
}