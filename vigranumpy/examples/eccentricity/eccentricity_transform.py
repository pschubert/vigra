import vigra
import numpy
import vigra.graphs as vigraph
import matplotlib.pyplot as plt
import scipy.misc
import sys


gamma = 0.0001
percentage = 2
f = "figure_1.png"






## img: image segment with 0: inside, 1: outside
## distFunc: function applied after distance transform, must be one of "exponential", "linear", "inverse"
## showPathImage: if True, the image with distance transform and paths will be shown
## percentageOfPaths: percentage of computed paths
def eccentricity( img, distFunc = "exponential", showPathImage = False, percentageOfPaths = 100, imgSaveName = "" ):
    ## Enlarge image by one pixel on each side
    img = img.astype(numpy.uint8)
    bigImg = numpy.ones( (img.shape[0]+2, img.shape[1]+2) )
    bigImg[1:bigImg.shape[0]-1, 1:bigImg.shape[1]-1] = img

    ## Find borders in img (replace with graph functions)
    borderImg = numpy.zeros(bigImg.shape)
    for y in range(bigImg.shape[1]-1):
        for x in range(bigImg.shape[0]-1):
            if bigImg[x,y] == 0:
                if bigImg[x+1,y] == 1 or bigImg[x,y+1] == 1:
                    borderImg[x, y] = 1
            else:
                if bigImg[x+1,y] == 0:
                    borderImg[x+1, y] = 1
                if bigImg[x,y+1] == 0:
                    borderImg[x, y+1] = 1

## regionImageToCrackEdgeImage ( labelImage )


    # ## Apply distanceTransform and modify (outside: high values, inside: low values)
    # distImage = vigra.filters.distanceTransform2D(bigImg.astype(numpy.float32))
    # if showPathImage:
    #     imgp = distImage.copy()
    # if distFunc == "exponential":
    #     distImage = numpy.exp(distImage*-gamma)
    # elif distFunc == "linear":
    #     maxDist = distImage.max()
    #     distImage = maxDist - distImage
    # elif distFunc == "inverse":
    #     w = numpy.where(distImage!=0)
    #     distImage[w] = 1/distImage[w]
    # else:
    #     print "wrong parameters for distFunc in eccentricity"

    ## Distance in the inside between two pixels is 1.0
    distImage = bigImg.copy().astype(numpy.float32)
    distImage[numpy.where(bigImg==0)]=1.0

    ## Set the outside to a very high value
    distImage[numpy.where(bigImg==1)]=10000.0

    imgp = distImage.copy()

    ## Get image graph and its path finder
    gridGraph = vigraph.gridGraph(bigImg.shape[0:2],False)
    edgeWeights = vigra.resize(distImage,[distImage.shape[0]*2-1,distImage.shape[1]*2-1],order=0)
    edgeWeights = vigra.graphs.edgeFeaturesFromInterpolatedImageCorrected(gridGraph,edgeWeights)
    pathFinder = vigraph.ShortestPathPathDijkstra(gridGraph)

    ## End points for paths (all points on the border)
    targets = numpy.where(borderImg==1)
    tx,ty = targets
    nTargets = len(tx)

    ## Indices of start points for paths (random)
    nPoints = int(numpy.ceil(percentageOfPaths * nTargets / 100.0))
    numpy.random.seed(42)
    starts = numpy.random.permutation(range(nTargets))[:nPoints]

    ## Compute paths
    maxPaths = []
    maxPathLengths = []
    for i in range(nPoints):
        source = gridGraph.coordinateToNode((int(tx[starts[i]]), int (ty[starts[i]])))
        pathFinder.run(edgeWeights, source)
        maxPathLength = 0
        for j in range(nTargets):
            target = gridGraph.coordinateToNode((int(tx[j]), int(ty[j])))
            path = pathFinder.path(pathType='coordinates', target=target)
            pathLength = pathFinder.distance(target)
            if pathLength > maxPathLength or maxPathLength == 0:
                maxPathLength = pathLength
                maxPath = path
        maxPaths.append(maxPath)
        maxPathLengths.append(maxPathLength)

    if showPathImage or len(imgSaveName)>1:
        val = (imgp.max()+imgp.min())/2
        for p in maxPaths:
            imgp[p[:,0], p[:,1]] = val
    if showPathImage:
        plt.figure(distFunc)
        plt.imshow(imgp, interpolation='none')
    if len(imgSaveName)>1:
        scipy.misc.imsave(imgSaveName, imgp)

    return maxPathLengths


## Read image
img = vigra.impex.readImage(f)
labels = numpy.squeeze(vigra.analysis.labelImage(img))

### Compute slic superpixels
#labels ,nseg = vigra.analysis.slicSuperpixels(img,100.0,50)
#labels       = numpy.squeeze(vigra.analysis.labelImage(labels))

## Compute bounding boxes
regionFeatures = vigra.analysis.extractRegionFeatures(img, labels)
upperLeftBBs = regionFeatures["Coord<Minimum>"]
lowerRightBBs = regionFeatures["Coord<Maximum>"]
nBoxes = len(upperLeftBBs)-1

## Get segment inside its bounding box
segments = []
nonEmptyBoxIndices = []
for i in range(nBoxes):
    subImg = labels[ upperLeftBBs[i+1][0]:lowerRightBBs[i+1][0], upperLeftBBs[i+1][1]:lowerRightBBs[i+1][1] ].copy()
    where = numpy.where(subImg==i+1)
    if len(where[0]) > 0:
        subImg[where] = 0
        subImg[numpy.where(subImg!=0)] = 1
        segments.append(subImg)
        nonEmptyBoxIndices.append(i+1)

## Apply eccentricity transform
pathLengths = []
counter = 0
for seg in segments:
    #eccentricity(subImg, distFunc="exponential", showPathImage=True, percentageOfPaths=percentage)
    #eccentricity(subImg, distFunc="inverse", showPathImage=True, percentageOfPaths=percentage)
    pathLength = eccentricity(seg, distFunc="linear", showPathImage=False, percentageOfPaths=percentage)
    pathLengths.append(pathLength)
    counter = counter+1
    #vigra.show()

# ## Testimage: map longest path to color
# maxPath = 0
# for i in range(len(pathLengths)):
#     m = max(pathLengths[i])
#     if m > maxPath:
#         maxPath = m
# labelCopy = labels.copy()
# for i in range(len(pathLengths)):
#     val = max(pathLengths[i]) * 255.0/maxPath
#     j = nonEmptyBoxIndices[i]
#     labelCopy[numpy.where(labels == j)] = val
#
# vigra.imshow(labelCopy)
# vigra.show()
