<?php

include("gauss.php");

// Sample template file that imports all functionality
// from the GAUSS library. Multiple callbacks are
// set up for testing purposes. User-defined code
// can be placed in the specificed block below,
// allowing quick and easy testing of code snippets
// from the documentation or any others you wish to try.

class Output extends IGEProgramOutput {
    function invoke($output) {
        echo $output;
    }
}

class FlushOutput extends IGEProgramFlushOutput {
    function invoke() {
        echo "A flush was requested.";
    }
}

class StringInput extends IGEProgramInputString {
    function invoke($length) {
        $this->setValue("Hello World!");
    }
}

class CharInput extends IGEProgramInputChar {
    function invoke() {
        return ord("a");
    }
}

class InputCheck extends IGEProgramInputCheck {
    function invoke() {
        # We pretend we have character input available.
        return 1;
    }
}

echo "[*] Instantiating GAUSS Engine" . PHP_EOL;

$ge = new GAUSS();

# Set output callback;
$out = new Output();
$out->thisown = 0;
$ge->setProgramOutputAll($out);

# Set output flush callback;
$flush = new FlushOutput();
$flush->thisown = 0;
$ge->setProgramFlushOutput($flush);

// Set string input callback
$inString = new StringInput();
$inString->thisown = 0;
$ge->setProgramInputString($inString);

// Set character input callback
$inChar = new CharInput();
$inChar->thisown = 0;
$ge->setProgramInputCharBlocking($inChar);
$ge->setProgramInputChar($inChar);

// Set character input check callback
$inputCheckCallback = new InputCheck();
$inputCheckCallback->thisown = 0;
$ge->setProgramInputCheck($inputCheckCallback);

echo "[*] GAUSS Engine callbacks successfully created and assigned" . PHP_EOL;

# Initialize the engine
if (!$ge->initialize()) {
    echo "[-] GAUSS Engine failed to initialize" . PHP_EOL;
    return;
}

echo "[*] GAUSS Engine initialized successfully" . PHP_EOL;

/////////////////////////////////
//     Code Snippet Template   //
//                             //
//    Insert your code here    //
//                             //
/////////////////////////////////

// Create a 2x3x4 array with values 1.0 - 24.0
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4)");
$ge->executeString("b = areshape(seqa(25, 1, 24), 2|3|4)");
$ge->executeString("c = complex(a, b)");
$c = $ge->getArray("c");
echo implode(", ", $c->getData()) . PHP_EOL;

echo "[*] Trying to shut down GAUSS Engine" . PHP_EOL;

$ge->shutdown();

echo "[*] GAUSS Engine has been shutdown" . PHP_EOL;

?>
