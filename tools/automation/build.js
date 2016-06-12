
var config = require('./config.json');
var fs = require('fs');
var path = require('path');
var execSync = require('child_process').execSync;
var ajgenesis = require('ajgenesis');

ajgenesis.fs.createDirectory('build');

processCkpool();

function processCkpool() {   

    var model = config.ckpool;
    
    ajgenesis.fileTransform(path.join('templates', 'ckpool.conf.tpl'), path.join('build', 'ckpool', 'run', 'conf', 'ckpool.conf'), model);
    //ajgenesis.fileTransform('run.sh.tpl', path.join('build', 'run.sh'), { });
}

