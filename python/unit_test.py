import random
import unittest
import sys
from ge import *

# Unit Test to Check functionality

class Output(IGEProgramOutput):
    def __init__(self):
        super().__init__()
        print("Creating output class")

    def invoke(self, output):
        print(output, end='', flush=True)

    def __del__(self):
        print("Deleting output class")

class FlushOutput(IGEProgramFlushOutput):
    def __init__(self):
        super().__init__()

    def invoke(self):
        print("A flush was requested.")

class StringInput(IGEProgramInputString):
    def __init__(self):
        super().__init__()

    # length is the maximum accepted string length
    def invoke(self, length):
        self.setValue("Hello World!")

class CharInput(IGEProgramInputChar):
    def __init__(self):
        super().__init__()

    def invoke(self):
        return ord('a')

class InputCheck(IGEProgramInputCheck):
    def __init__(self):
        super().__init__()

    def invoke(self):
        # We pretend we have character input available.
        print("Input check requested")
        return 1

eng = GAUSS()
eng.setOutputModeManaged(False)

#thisown = 0

# Set output callback
out = Output() #.__disown__()
eng.setProgramOutputAll(out)

# Set output flush callback
flush = FlushOutput().__disown__()

eng.setProgramFlushOutput(flush)

# Set string input callback
inString = StringInput().__disown__()

eng.setProgramInputString(inString)

# Set character input callback
inChar = CharInput().__disown__()

eng.setProgramInputCharBlocking(inChar)
eng.setProgramInputChar(inChar)

# Set character input check callback
inputCheckCallback = InputCheck().__disown__()
#inputCheckCallback.thisown = 0;
eng.setProgramInputCheck(inputCheckCallback)

eng.initialize()

class TestGAUSSEngine(unittest.TestCase):
    ge = eng

#    def setUp(self):
#        self.ge = GAUSS()
#        self.ge.setOutputModeManaged(False)
#
#        self.assertTrue(self.ge is not None)
#
#        #thisown = 0
#
#        # Set output callback
#        self.out = Output() #.__disown__()
#        self.ge.setProgramOutputAll(self.out)
#
#        # Set output flush callback
#        flush = FlushOutput().__disown__()
#        
#        self.ge.setProgramFlushOutput(flush)
#
#        # Set string input callback
#        inString = StringInput().__disown__()
#        
#        self.ge.setProgramInputString(inString)
#
#        # Set character input callback
#        inChar = CharInput().__disown__()
#        
#        self.ge.setProgramInputCharBlocking(inChar)
#        self.ge.setProgramInputChar(inChar)
#
#        # Set character input check callback
#        inputCheckCallback = InputCheck().__disown__()
#        #inputCheckCallback.thisown = 0;
#        self.ge.setProgramInputCheck(inputCheckCallback)
#
#        self.assertTrue(self.ge.initialize())

    def testMatrices(self):
        self.ge.executeString("x = 5")
        x = self.ge.getMatrix("x")
        self.assertEquals(x.getElement(), 5)

        self.ge.executeString("x = { 1 2, 3 4 }")
        x = self.ge.getMatrixAndClear("x")
        self.assertEquals(x.getRows(), 2)
        self.assertEquals(x.getCols(), 2)
        self.assertEquals(list(x.getData()), [1, 2, 3, 4])

        x2 = self.ge.getMatrix("x")
        self.assertEquals(x2.getRows(), 1)
        self.assertEquals(x2.getCols(), 1)
        self.assertEquals(x2.getElement(), 0)

        self.ge.executeString("x = complex(seqa(1,1,8), seqa(9,1,8))")
        x = self.ge.getMatrix("x")
        self.assertNotEquals(x, None)
        self.assertEquals(len(x.getData(True)), 16)

        self.assertEquals(list(x.getData()), [1, 2, 3, 4, 5, 6, 7, 8])
        self.assertEquals(list(x.getImagData()), [9, 10, 11, 12, 13, 14, 15, 16])

        self.ge.setSymbol(GEMatrix(range(0, 10)), "y")
        y = self.ge.getMatrix("y")
        self.assertEquals(list(y.getData()), [float(i) for i in range(0, 10)])
        pass

    def testArrays(self):
        self.ge.executeString("ai = seqa(1,1,24); aj = seqa(25,1,24);")

        # Create a complex 3x3x3 array
        self.ge.executeString("as = areshape(ai, 2|3|4); at = areshape(aj, 2|3|4); au = complex(as,at)")

        # Retrieve the array from the symbol table
        auArray = self.ge.getArray("au")

        self.assertEquals(list(auArray.getOrders()), [2, 3, 4])
        self.assertEquals(auArray.getDimensions(), 3)

        self.assertEquals(auArray.size(), 24) # complex
        self.assertEquals(list(auArray.getData()), [float(i) for i in range(1, 25)])
        self.assertEquals(list(auArray.getImagData()), [float(i) for i in range(25, 49)])

        # Extract a plane from the array
        self.assertEquals(list(auArray.getPlane([0, 2, 0]).getData()), [5, 6, 7, 8, 17, 18, 19, 20])
        self.assertEquals(list(auArray.getVector([0, 2, 2])), [6, 18])

        # We are directly passing in the orders, followed by the data
        a2 = GEArray([2, 2, 2], [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0])

        # Add it to the symbol table
        self.ge.setSymbol(a2, "a2")

        a3 = self.ge.getArrayAndClear("a2")
        self.assertEquals(list(a3.getOrders()), [2, 2, 2])
        self.assertEquals(a3.getDimensions(), 3)

        a2 = self.ge.getArray("a2")
        self.assertEquals(list(a2.getOrders()), [1])
        self.assertEquals(a2.getDimensions(), 1)
        self.assertEquals(list(a2.getData()), [0])

    def testStrings(self):
        geStr = GEString("Hello World")

        # Add it to the symbol table
        self.ge.setSymbol(geStr, "s")

        s = self.ge.getString("s")
        self.assertEquals(str(s), "Hello World")

        # create a temp workspace
        tempWh = self.ge.createWorkspace("temp")

        # Change the value of 's' in GAUSS
        self.ge.executeString("s = \"Goodbye World\"", tempWh)
        self.ge.executeString("print s", tempWh)

        s = self.ge.getString("s")
        self.assertEquals(str(s), "Hello World")

        s = self.ge.getString("s", tempWh)
        self.assertEquals(str(s), "Goodbye World")

        # Here we can verify that the symbol type of 's' is in fact a string
        self.assertEquals(self.ge.getSymbolType("s"), GESymType.STRING)
        self.assertEquals(self.ge.getSymbolType("s", tempWh), GESymType.STRING)

        # Retrieve it from the symbol table
        s = self.ge.getString("s")
        self.assertEquals(s.size(), 11)

    def testStringArrays(self):
        # Create an example string array using GAUSS code.
        self.ge.executeString("string sa = { this is a test, one two three four, five six seven eight }")

        # Retrieve the string array from the symbol table
        sa = self.ge.getStringArray("sa")

        self.assertNotEquals(sa, None)

        self.assertEquals(sa.size(), 12)

        self.assertEquals(sa.getElement(2,3), "EIGHT")

        # Create a string array in PHP
        sa2 = ["one", "two", "three", "four"]

        gesa2 = GEStringArray(sa2, 2, 2)

        # Add it to the symbol table
        self.ge.setSymbol(gesa2, "sa2")

        sa = self.ge.getStringArray("sa2")
        self.assertEquals(sa.size(), 4)
        self.assertEquals(list(sa.getData()), ["one", "two", "three", "four"])

#    def tearDown(self):
#        self.ge.shutdown()

suite = unittest.TestLoader().loadTestsFromTestCase(TestGAUSSEngine)
unittest.TextTestRunner(verbosity=3).run(suite)

