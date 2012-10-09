Part of the SOK project http://community.kde.org/Digikam/SoK2012/AutoNR

1. This is a code sample which takes in a input of a image file.
2. Converts the file to YCrCb Image format.
3. Divides the image to less than equal to Thirty Clusters.
4. Calculates mean and std of each cluster.
5. Gives in a output of the weighted mean all clusters of each of the channels.
6. We infer the Standard deviation of each image and calculate the threshold and the Softness.

Information about the Standard Deviation data obtained:
1. Standard Deviation ~ 2.0 to 3.0 (No Noise)
2. Standard Deviation ~ >6.0 (heavy noise)

Parts remaining:
1. Adaptive process from standard deviation float data to threshold and Softness for each of the channels.

Parts not covered :
1. Images only of type png, jpeg, jpg works. Does not work with raw file format of cameras (e.g. .NEF, .CR2)
