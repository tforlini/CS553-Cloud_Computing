# Random integer generator
import random

afile = open("Random.txt", "w" )

for i in range(input('How many random numbers?: ')):
    line = str(random.randint(0, 10000))
    afile.write(line+str("\n"))
    print(line)

afile.close()


print("\nReading the file now." )
afile = open("Random.txt", "r")

print(afile.read())
afile.close()
