var tone = require("tonegenerator");
var header = require("waveheader");
var fs = require('fs');

var numSeconds = 20;
var tones = [];

for (var i = 0; i < numSeconds; i++) {
    var tones = tones.concat(tone(20, 1));
}

console.log(tones.length);

var writer = new fs.createWriteStream("tones/20.wav");
writer.write(header( 44100 * 20 )); // 44100 Hz * 8 seconds
writer.write(new Buffer(tones));
writer.end();