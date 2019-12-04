var ge;

if (process.env.DEBUG) {
  ge = require('./build/Debug/ge.node');
} else {
  ge = require('./build/Release/ge.node');
}

module.exports = ge;

