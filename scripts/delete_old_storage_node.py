import nuke

# Deletes the old _MarkerTimebar_ NoOp storage node.
# Safe to run — the new version stores data on the Root node instead.
# Any markers already saved in the NoOp will be lost, so re-add them
# after running this script if needed.

node = nuke.toNode('_MarkerTimebar_')
if node:
    nuke.delete(node)
    print("Deleted _MarkerTimebar_ NoOp node.")
    print("Marker data is now stored on the Root node (invisible to the DAG).")
    print("The F key zoom-out bug should be resolved after saving the script.")
else:
    print("No _MarkerTimebar_ node found — nothing to delete.")
