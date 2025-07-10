# HPC_Assignement

## AddNoise.sh
In the directory "../input/reference" with respect to the one you run this script, load all the files you want to add Noise. 
Create directories "..input/noiseXX" where XX is 50, 75, 90. You will find there the result of the image in which you add your noise

## clean.sh
Used to clean all the data collected in the directory ../logData

## Es2
The executable file must be launched with two input arguments: the first one must be the image you want to process, the second one is the name of the image as output. You can add a third argument "-s", in this case you will also save the image. All the time data collected are stored in the file "../logData/logFile" in append mode.

## Es2.sh
This is the script that compiles and run Es2.cu. You have to provide the input and output directories from which you want to collect data and to which you want to save them. It doesn't save the result image. It also saves memory usage data in this directory: "../logData/nsysProfile"
