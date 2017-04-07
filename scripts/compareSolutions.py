try:
    import commentjson as json
except ImportError:
    import json

import argparse
    
def extractEventLists(solution):
    divisions = solution["divisionResults"]
    moves = solution["linkingResults"]
    detections = solution["detectionResults"]
    
    if divisions is None:
        divisions = []
    
    detEvents = set([o["id"] for o in detections if o['value'] > 0])
    moveEvents = set([(o["src"], o["dest"]) for o in moves if o['value'] > 0])
    try:
        divEvents = set([(o["parent"], o["children"][0], o["children"][1]) for o in divisions if o['value']])
    except KeyError:
        divEvents = set([o["id"] for o in divisions if o['value']])
    
    return detEvents, moveEvents, divEvents, len(detEvents), len(moveEvents), len(divEvents)

def precision(tp, fp):
    return float(tp) / (tp + fp)

def recall(tp, fn):
    return float(tp) / (tp + fn)
    
def fmeasure(precision, recall):
    return 2.0 * (precision * recall) / (precision + recall)

def printStats(n, tp, fp, fn, numEvents, numPos):
    if numEvents > 0:
        p = precision(tp, fp)
        r = recall(tp, fn)
        f = fmeasure(p, r)
    else:
        p,r,f = 0,0,0
    print("\n=== {} ===".format(n))
    print("\t{} gt events, {} in result".format(numEvents, numPos))
    print("\tprecision: {}".format(p))
    print("\trecall: {} ".format(r))
    print("\tf-measure: {}".format(f))

#%%
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
        Compare two result json files, usually one of those is the ground truth...
        """, formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--gt', dest='gtFilename', type=str, required=True, 
                        help='Filename of the ground truth result file')
    parser.add_argument('--result', dest='resultFilename', type=str, required=True, 
                        help='Filename of the JSON file of the results')
    args = parser.parse_args()
    
    # gtFilename = '/Users/chaubold/hci/data/virginie/test/generatedGroundTruth_withLoG.json'
    # resultFilename = '/Users/chaubold/hci/data/virginie/test/result.json'
    
    gtFilename = args.gtFilename
    resultFilename = args.resultFilename
    
    with open(gtFilename, 'r') as f:
        gt = json.load(f)
    
    with open(resultFilename, 'r') as f:
        result = json.load(f)
    
    gtEvents = extractEventLists(gt)
    resultEvents = extractEventLists(result)
    
    #%%
    totalTp = 0    
    totalFp = 0
    totalFn = 0
    totalGtEvents = 0
    totalResultEvents= 0
    
    for i,n in enumerate(['detections', 'moves', 'divisions']):
        try:
            tp = len(gtEvents[i].intersection(resultEvents[i]))
            fp = len(resultEvents[i].difference(gtEvents[i]))
            fn = len(gtEvents[i].difference(resultEvents[i]))
            # tn = gtEvents[i + 3] - tp - fp - fn

            printStats(n, tp, fp, fn, gtEvents[i + 3], resultEvents[i + 3])
            totalTp += tp
            totalFp += fp
            totalFn += fn
            totalGtEvents += gtEvents[i + 3]
            totalResultEvents += resultEvents[i + 3]
        except ZeroDivisionError:
            print("\n\nDid not find {} results".format(n))
    
    print('\n=======================')
    printStats('overall', totalTp, totalFp, totalFn, totalGtEvents, totalResultEvents)
    
