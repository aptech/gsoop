var ge = require('./');

/*
// For waiting to attach debugger.
var fs = require("fs")
var fd = fs.openSync("/dev/stdin", "rs")
fs.readSync(fd, new Buffer(1), 0, 1)
fs.closeSync(fd)
*/

var obj = new ge.GAUSS();

var out = new ge.GEOutput(function(output) { console.log('inline out -> ' + output); });
ge.GAUSS.setOutputModeManaged(false);
ge.GAUSS.setProgramOutputAll(out);

obj.initialize();

async function f1() {

  let promise = new Promise((resolve, reject) => {
    let wh = obj.createWorkspace("foo");
    let out = new ge.GEOutput(function(output) { console.log('out #1 -> ' + output); });
    ge.GAUSS.setProgramOutputAll(out);
    obj.executeString("x = reshape(seqa(1,1,10), 5, 2)", wh);
    obj.executeString("print x", wh);

    let x = obj.getMatrix("x", wh);
    obj.destroyWorkspace(wh);
    resolve(x);
  });

  return promise;
}

async function f2() {

  let promise = new Promise((resolve, reject) => {
    let wh = obj.createWorkspace("bar");
    let out = new ge.GEOutput(function(output) { console.log('out #2 -> ' + output); });
    ge.GAUSS.setProgramOutputAll(out);
    obj.executeString("x = reshape(seqa(25,5,10), 5, 2)", wh);
    obj.executeString("print x", wh);

    let x = obj.getMatrix("x", wh);
    obj.destroyWorkspace(wh);
    resolve(x);
  });

  return promise;
}

f1().then(function(result) {
  for (let n of result.getData()) {
    console.log('F1: Value is = ' + n);
  }
});

f2().then(function(result) {
  for (let n of result.getData()) {
    console.log('F2: Value is = ' + n);
  }
});

obj.executeString("print \"Hello World\"");
//console.log(obj.getOutput());

obj.executeString("x = reshape(seqa(1,1,10), 5, 2)");

var x = obj.getMatrix("x");

obj.executeString('print rows(x) "x" cols(x);'); 
obj.executeString('rndu(10,5);');
//console.log('x size = ' + x.getRows() + 'x' + x.getCols());

for (let n of x.getData()) {
    console.log('Value is = ' + n);
}

obj.executeString("s = \"hello world\"");
obj.executeString("sa = reshape(s, 5, 5)");

var s = obj.getString("s");
var sa = obj.getStringArray("sa");
var sa_data = sa.getData();

obj.shutdown();

