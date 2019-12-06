
var ge = require('./');

var obj = new ge.GAUSS();

obj.initialize();
obj.executeString("print \"Hello World\"");
console.log(obj.getOutput());
obj.shutdown();

