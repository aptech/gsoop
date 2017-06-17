<?php
//declare(strict_types=1);

require_once("ge.php");

use PHPUnit\Framework\TestCase;

class Output extends IGEProgramOutput
{
    public function __construct() {
        parent::__construct();
        print("Creating output class\n");
    }

    public function invoke($output) {
        print($output);
    }
}

class FlushOutput extends IGEProgramFlushOutput
{
    public function __construct() {
        parent::__construct();
    }

    public function invoke() {
        print("A flush was requested.\n");
    }
}

class StringInput extends IGEProgramInputString
{
    public function __construct() {
        parent::__construct();
    }

    // length is the maximum accepted string length
    public function invoke($length) {
        $this->setValue("Hello World!");
    }
}

class CharInput extends IGEProgramInputChar
{
    public function __construct() {
        parent::__construct();
    }

    public function invoke() {
        return ord('a');
    }
}

class InputCheck extends IGEProgramInputCheck
{
    public function __construct() {
        parent::__construct();
    }

    public function invoke() {
        // We pretend we have character input available->
        print("Input check requested\n");
        return 1;
    }
}

final class TestGAUSSEngine extends TestCase
{
    protected static $ge;
    protected static $out;
    protected static $flush;
    protected static $inString;
    protected static $inChar;
    protected static $inputCheckCallback;

    public static function setUpBeforeClass()
    {
        //sleep(5);
        self::$ge = new GAUSS();
        self::$ge->setOutputModeManaged(false);
        
        $thisown = 1;
        
        // Set output callback
        self::$out = new Output();
        self::$out->thisown = $thisown;
        self::$ge->setProgramOutputAll(self::$out);
        
        // Set output flush callback
        self::$flush = new FlushOutput();
        self::$flush->thisown = $thisown;
        self::$ge->setProgramFlushOutput(self::$flush);
        
        // Set string input callback
        self::$inString = new StringInput();
        self::$inString->thisown = $thisown;
        
        self::$ge->setProgramInputString(self::$inString);
        
        // Set character input callback
        self::$inChar = new CharInput();
        self::$inChar->thisown = $thisown;
        
        self::$ge->setProgramInputCharBlocking(self::$inChar);
        self::$ge->setProgramInputChar(self::$inChar);
        
        // Set character input check callback
        self::$inputCheckCallback = new InputCheck();
        self::$inputCheckCallback->thisown = 0;
        self::$ge->setProgramInputCheck(self::$inputCheckCallback);
        
        self::$ge->initialize();

    }

    public function __construct() 
    {
        parent::__construct();
    }

    public function testWorkspaces()
    {
        $wh1 = self::$ge->createWorkspace("wh1");
        $wh11 = self::$ge->createWorkspace("wh1");
        $wh111 = self::$ge->createWorkspace("wh1");
        $wh1111 = self::$ge->createWorkspace("wh1");
        $this->assertEquals("wh1", $wh1->name());
        $this->assertTrue(self::$ge->destroyWorkspace($wh1));
        $this->assertEquals(NULL, $wh1->workspace());

        $this->assertFalse(self::$ge->destroyWorkspace($wh1));

        $wh2 = self::$ge->createWorkspace("wh2");
        self::$ge->setActiveWorkspace($wh2);
        $this->assertEquals($wh2->name(), self::$ge->getActiveWorkspace()->name());

        $wh3 = self::$ge->createWorkspace("wh3");
        $wh4 = self::$ge->createWorkspace("wh4");

        $wh2->setName("wh222");
        self::$ge->updateWorkspaceName($wh2);
        $this->assertEquals("wh2", $wh2->name());

        $count = 100;

        for ($i = 0; $i < $count; ++$i)
        {
            $t = self::$ge->createWorkspace("temp" . $i);
            self::$ge->executeString("print \"my random number: \" rndu(1,1) \" for \" " . $i, $t);
        }

        for ($i = 0; $i < $count; ++$i)
        {
            self::$ge->destroyWorkspace(self::$ge->getWorkspace("temp" . $i));
        }
    }


    public function testMatrices()
    {
        $wh = self::$ge->createWorkspace("testMatrices");
        self::$ge->executeString("x = 5", $wh);
        $x = self::$ge->getMatrix("x", $wh);
        $this->assertEquals(5, $x->getElement());

        self::$ge->executeString("x = { 1 2, 3 4 }", $wh);
        $x = self::$ge->getMatrixAndClear("x", $wh);
        $this->assertEquals(2, $x->getRows());
        $this->assertEquals(2, $x->getCols());
        $this->assertEquals(array(1, 2, 3, 4), $x->getData());

        $x2 = self::$ge->getMatrix("x", $wh);
        $this->assertEquals(1, $x2->getRows());
        $this->assertEquals(1, $x2->getCols());
        $this->assertEquals(0, $x2->getElement());

        self::$ge->executeString("x = complex(seqa(1,1,8), seqa(9,1,8))", $wh);
        $x = self::$ge->getMatrix("x", $wh);
        $this->assertNotEquals(NULL, $x);
        $this->assertEquals(16, count($x->getData(true)));

        $this->assertEquals(array(1, 2, 3, 4, 5, 6, 7, 8), $x->getData());
        $this->assertEquals(array(9, 10, 11, 12, 13, 14, 15, 16), $x->getImagData());

        $zero_to_ten = range(0,10);
        self::$ge->setSymbol(new GEMatrix($zero_to_ten), "y", $wh);
        $y = self::$ge->getMatrix("y", $wh);
        $this->assertEquals($zero_to_ten, $y->getData());
        $this->assertEquals(11, count($y));

        foreach ($zero_to_ten as $number) {
            $this->assertEquals($number, $y[$number]);
        }

        $c = 0.0;
        foreach ($y as $number) {
            $this->assertEquals($c, $number);
            $c += 1.0;
        }
    }

    public function testMatrixDirect() {
        $lowLevelMat1 = new doubleArray(10);
        for ($i = 0; $i < 10; ++$i)
        {
            $lowLevelMat1[$i] = $i + 1;
        }

        foreach ($lowLevelMat1 as $n) {
            echo "num = " . $n . PHP_EOL;
        }

        //$this->assertTrue(self::$ge->moveMatrix($lowLevelMat1, 5, 2, false, "x1"));
        self::$ge["x1"] = $lowLevelMat1;
        self::$ge->executeString("\"x1 = \" x1");
        $mat1Copy = self::$ge["x1"];
        var_dump($mat1Copy);
        $this->assertEquals(range(1, 10), $mat1Copy->getData());

        $this->assertTrue(self::$ge->executeString("x2 = x1 + 1"));

        $lowLevelMat1 = self::$ge->getMatrixDirect("x1");
        $lowLevelMat2 = self::$ge->getMatrixDirect("x2");
        for ($i = 0; $i < 10; ++$i)
        {
            $this->assertEquals($i + 2, $lowLevelMat2[$i]);
            $lowLevelMat1[$i] = $i + 2;
        }

        $this->assertTrue(self::$ge->executeString("x3 = (x2 == x1)"));
        $this->assertEquals(1, self::$ge->getScalar("x3"));

        $this->assertEquals(10, count($mat1Copy));

    }

    public function testArrays()
    {
        $wh = self::$ge->createWorkspace("testArrays");
        self::$ge->executeString("ai = seqa(1,1,24); aj = seqa(25,1,24);", $wh);

        // Create a complex 3x3x3 array
        self::$ge->executeString("as = areshape(ai, 2|3|4); at = areshape(aj, 2|3|4); au = complex(as,at)", $wh);

        // Retrieve the array from the symbol table
        $auArray = self::$ge->getArray("au", $wh);

        $this->assertEquals(array(2, 3, 4), $auArray->getOrders());
        $this->assertEquals(3, $auArray->getDimensions());

        $this->assertEquals(24, $auArray->size());
        $this->assertEquals(range(1.0, 24.0), $auArray->getData());
        $this->assertEquals(range(25.0, 48.0), $auArray->getImagData());

        // Extract a plane from the array
        $this->assertEquals(array(5, 6, 7, 8, 17, 18, 19, 20), $auArray->getPlane(array(0, 2, 0))->getData());
        $this->assertEquals(array(6, 18), $auArray->getVector(array(0, 2, 2)));

        // We are directly passing in the orders, followed by the data
        $a2 = new GEArray(array(2, 2, 2), array(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0));

        // Add it to the symbol table
        self::$ge->setSymbol($a2, "a2", $wh);

        $a3 = self::$ge->getArrayAndClear("a2", $wh);
        $this->assertEquals(array(2, 2, 2), $a3->getOrders());
        $this->assertEquals(3, $a3->getDimensions());

        $a2 = self::$ge->getArray("a2", $wh);
        $this->assertEquals(array(1), $a2->getOrders());
        $this->assertEquals(1, $a2->getDimensions());
        $this->assertEquals(array(0), $a2->getData());
    }

    public function testStrings()
    {
        $wh = self::$ge->createWorkspace("testStrings");
        self::$ge->setActiveWorkspace($wh);

        $geStr = "Hello World";

        // Add it to the symbol table
        self::$ge->setSymbol($geStr, "s", $wh);

        $s = self::$ge["s"][0];
        var_dump($s);
        $this->assertEquals("H", $s[0]);

        //$this->assertEquals("e", $s[0][1]);

        $s = self::$ge->getString("s", $wh);
        $this->assertEquals("Hello World", $s);

        // create a temp workspace
        $tempWh = self::$ge->createWorkspace("temp");

        // Change the value of 's' in GAUSS
        self::$ge->executeString("s = \"Goodbye World\"", $tempWh);
        self::$ge->executeString("print s", $tempWh);

        $s = self::$ge->getString("s", $wh);
        $this->assertEquals("Hello World", $s);

        $s = self::$ge->getString("s", $tempWh);
        $this->assertEquals("Goodbye World", $s);

        // Here we can verify that the symbol type of 's' is in fact a string
        $this->assertEquals(GESymType::STRING, self::$ge->getSymbolType("s", $wh));
        $this->assertEquals(GESymType::STRING, self::$ge->getSymbolType("s", $tempWh));

        // Retrieve it from the symbol table
        $s = self::$ge->getString("s", $wh);
        $this->assertEquals(11, strlen($s));
    }

    public function testStringArrays()
    {
        $wh = self::$ge->createWorkspace("testStringArrays");
        // Create an example string array using GAUSS code->
        //
        self::$ge->executeString("string sa = { this is a test, one two three four, five six seven eight }", $wh);

        // Retrieve the string array from the symbol table
        $sa = self::$ge->getStringArray("sa", $wh);

        $this->assertNotEquals(NULL, $sa);

        $this->assertEquals(12, $sa->size());

        $this->assertEquals("EIGHT", $sa->getElement(2,3));

        // Create a string array in PHP
        $sa2 = array("one", "two", "three", "four");

        $gesa2 = new GEStringArray($sa2, 2, 2);

        // Add it to the symbol table
        self::$ge->setSymbol($gesa2, "sa2", $wh);

        $sa = self::$ge->getStringArray("sa2", $wh);
        $this->assertEquals(4, $sa->size());

        $t = array("one", "two", "three", "four");
        $this->assertEquals($t, $sa->getData());

        $n = 0;

        foreach ($sa as $s) {
            $this->assertEquals($t[$n], $s);
            ++$n;
        }

        $this->assertEquals(4, count($sa));

        foreach (range(0,3) as $n) {
            $this->assertEquals($t[$n], $sa[$n]);
        }
    }
}

?>
