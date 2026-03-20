import nuke

# Find and select the old _MarkerTimebar_ NoOp storage node.
# Run this in the Nuke Script Editor if the F key zoom-out bug is present.

node = nuke.toNode('_MarkerTimebar_')
if node:
    nuke.selectAll()
    nuke.invertSelection()
    node['selected'].setValue(True)
    nuke.zoomToFitSelected()
    print("Found _MarkerTimebar_ at: {}, {}".format(node.xpos(), node.ypos()))
    print("Node is now selected and framed in the DAG.")
    print("Press Delete to remove it, or run delete_old_storage_node.py")
else:
    print("No _MarkerTimebar_ node found in this script — nothing to do.")
