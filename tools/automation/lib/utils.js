
var spawn = require('child_process').spawn;
var exec = require('child_process').exec;
var Client = require('ssh2').Client;
var scp = require('scp2');
var fs = require('fs');

function executeCommand(cmd, cb) {
    var words = cmd.split(' ');
    
    var verb = words[0];
    var args = words.slice(1);
    
    var proc = spawn(verb, args);
    
    proc.stdout.on('data', function (data) {
        console.log(data.toString());
    });
    
    proc.stderr.on('data', function (data) {
        console.log(data.toString());
    });
    
    proc.on('close', function (code) {
        if (code)
            cb('Error ' + code, null);
        else
            cb(null, null);
    });
    
    proc.on('error', function (err) {
        cb(err, null);
    });
};

function getJson(cmd, cb) {
    exec(cmd, function (error, stdout, stderr) {
        if (error)
            return cb(error, null);
        
        try {
            var json = JSON.parse(stdout.toString());
            cb(null, json);
        }
        catch (ex) {
            cb(ex, null);
        }
    });
}

function getMachineInfo(artifactName, config) {

    var machineName = config.deploy.artifacts[artifactName];

    var machine = config.machines.filter(function(machine){
        return machine.name == machineName;
    })[0];

    return machine;
}

function addSshConnectionInfo(machine, argv, config) {
    
    machine.ssh = 
    {
        "user" : argv.user,
        "privateKey" : 
        {
            "directory": config.ssh.privateKeyDirectory,
            "passphrase": argv.passphrase
        }
    };
}

function executeRemoteCommand(machine, cmd, cb) {
    
    var conn = new Client();
    
    conn.on('ready', function() {
      //console.log('Client :: ready');
      conn.exec(cmd, function(err, stream) {
        if (err) throw err;

        stream.on('close', function(code, signal) {
          //console.log('Stream :: close :: code: ' + code + ', signal: ' + signal);
          conn.end();
          cb(null, null);
        }).on('data', function(data) {
          console.log(data.toString());
        }).stderr.on('data', function(data) {
          console.log(data.toString());
          //cb(data, null);
        });
      });
    }).connect({
        host: machine.public.ip || machine.private.ip,
        port: machine.public.port || machine.private.port,
        username: machine.ssh.user,
        privateKey: fs.readFileSync(machine.ssh.privateKey.directory),
        passphrase: machine.ssh.privateKey.passphrase,
    });
}

function copyRemoteCommand(machine, source, destination, cb) {

    console.log("Copying files from " + "localhost:" + source + " to " + machine.name + ":" + destination);

    var receiverInfo = {
        host: machine.public.ip || machine.private.ip,
        port: machine.public.port || machine.private.port,
        path: destination,
        username: machine.ssh.user,
        privateKey: fs.readFileSync(machine.ssh.privateKey.directory),
        passphrase: machine.ssh.privateKey.passphrase,
    };

    scp.scp(source, receiverInfo, function (err) {
        if (err)
            return cb(err, null);
        
        cb(null, null);
    });   
}

function downloadRemoteCommand(machine, source, destination, cb) {

    console.log("Copying files from " + machine.name + ":" + source + " to " + "localhost" + ":" + destination);

    var sourceInfo = {
        host: machine.public.ip || machine.private.ip,
        port: machine.public.port || machine.private.port,
        path: source,
        username: machine.ssh.user,
        privateKey: fs.readFileSync(machine.ssh.privateKey.directory),
        passphrase: machine.ssh.privateKey.passphrase,
    };

    scp.scp(sourceInfo, destination, function (err) {
        if (err)
            return cb(err, null);

        cb(null, null);
    });   
}

module.exports = {
    execute: executeCommand,
    executeRemote: executeRemoteCommand,
    copyRemote: copyRemoteCommand,
    downloadRemote: downloadRemoteCommand,
    getMachineInfo: getMachineInfo,
    addSshConnectionInfo: addSshConnectionInfo
};

