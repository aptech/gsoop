
var ge = require('./');

var obj = new ge.GAUSS();

obj.initialize();
obj.executeString("print \"Hello World\"");
obj.executeString("x = reshape(seqa(1,1,40), 10, 4);");
var x = obj.getMatrixDirect("x");

console.log('Testing streaming row data with getMatrixDirect + getrow()');

for (let i = 0; i < x.rows(); ++i) {
    console.log(x.getrow(i));
}

console.log(obj.getOutput());
obj.shutdown();

