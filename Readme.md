# Multi Hypothesis Tracking
by Carsten Haubold, 2015

This is a standalone tool for running tracking of divisible objects, with competing detection hypotheses in each frame.
When specifying a ground truth labeling for a dataset, the weights can be learned using structured learning 
(by [OpenGM's](http://github.com/opengm/opengm) implementation of [SBMRM](https://github.com/funkey/sbmrm)).
The tracking problem is then solved as ILP by Gurobi or CPLEX, depending on how this tool was compiled.

## JSON file formats

* Graph description: [test/magic.json](test/magic.json)
	- arbitrary number of features allowed (in `[]`), their weights 
* Tracking Result = Ground Truth format: [test/gt.json](test/gt.json)
* Weight result format: [test/weights.json](test/weights.json)

## Dot output

After calling `Model::toDot(filename, solution)`, the resulting `*.dot` file can be transformed to e.g. PDF by invoking:

```
dot -Tpdf result.dot > result.pdf
```

The graph should then look as below, where blue nodes and edges indicate that they were used in the solution, 
black ones are unused, and red edges display mutual exclusions.
![Result Graph](test/result.png)