import sys
from ge import *

# Sample template file that imports all functionality
# from the GAUSS library. Multiple callbacks are
# set up for testing purposes. User-defined code
# can be placed in the specificed block below,
# allowing quick and easy testing of code snippets
# from the documentation or any others you wish to try.

class Output(IGEProgramOutput):
    def invoke(self, output):
        print(output, end='', flush=True)

class FlushOutput(IGEProgramFlushOutput):
    def invoke(self):
        print("A flush was requested.")

class StringInput(IGEProgramInputString):
    # length is the maximum accepted string length
    def invoke(self, length):
        self.setValue("Hello World!")

class CharInput(IGEProgramInputChar):
    def invoke(self):
        return ord('a')

class InputCheck(IGEProgramInputCheck):
    def invoke(self):
        # We pretend we have character input available.
        print("Input check requested")
        return 1

print("[*] Instantiating GAUSS Engine")

ge = GAUSS()

# Set output callback
out = Output()
out.thisown = 0
ge.setProgramOutputAll(out)

# Set output flush callback
flush = FlushOutput()
flush.thisown = 0
ge.setProgramFlushOutput(flush)

# Set string input callback
inString = StringInput()
inString.thisown = 0
ge.setProgramInputString(inString)

# Set character input callback
inChar = CharInput()
inChar.thisown = 0
ge.setProgramInputCharBlocking(inChar)
ge.setProgramInputChar(inChar)

# Set character input check callback
inputCheckCallback = InputCheck()
inputCheckCallback.thisown = 0
ge.setProgramInputCheck(inputCheckCallback)

print("[*] GAUSS Engine callbacks successfully created and assigned")

# Initialize the engine
if not ge.initialize():
    print("[-] GAUSS Engine failed to initialize")
    sys.exit(1)

print("[*] GAUSS Engine initialized successfully")

###############################
#     Code Snippet Template   #
#                             #
#    Insert your code here    #
#                             #
###############################

# Create a 2x3x4 array with values 1.0 - 24.0
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")
ge.executeString("b = areshape(seqa(25, 1, 24), 2|3|4)")
ge.executeString("c = complex(a, b)")
c = ge.getArray("c")
print(", ".join(str(n) for n in c.getData()))

###############################

#print("[*] Trying to shut down GAUSS Engine")
#ge.shutdown()

ge.executeString("x = seqa(1,1,20)")
x = ge.getMatrix("x")

print("[*] GAUSS Engine has been shutdown")
