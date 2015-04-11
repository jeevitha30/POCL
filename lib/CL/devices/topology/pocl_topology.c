/* pocl_topology.c - retrieving the topology of OpenCL devices

   Copyright (c) 2012 Cyril Roelandt and Pekka Jääskeläinen
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <pocl_cl.h>
#include <hwloc.h>

#include "pocl_topology.h"

void
pocl_topology_detect_device_info(cl_device_id device)
{
  hwloc_topology_t pocl_topology;

  int ret = hwloc_topology_init(&pocl_topology);
  if (ret == -1)
    POCL_ABORT("Cannot initialize the topology.\n");

  /* we also want to get the IO bridge, to extract the device vendor id,
   * if possible
   */
  hwloc_topology_set_flags(pocl_topology, HWLOC_TOPOLOGY_FLAG_WHOLE_IO);
  ret = hwloc_topology_load(pocl_topology);
  if (ret == -1)
    POCL_ABORT("Cannot load the topology.\n");

  device->global_mem_size = hwloc_get_root_obj(pocl_topology)->memory.total_memory;

  // Try to get the number of CPU cores from topology
  int depth = hwloc_get_type_depth(pocl_topology, HWLOC_OBJ_PU);
  if(depth != HWLOC_TYPE_DEPTH_UNKNOWN)
    device->max_compute_units = hwloc_get_nbobjs_by_depth(pocl_topology, depth);

  // A vendor ID for a CPU is not well-defined, so we just use the
  // PCI vendor ID of a bridge, on the (debatable) assumption that it matches
  // the CPU vendor (e.g. AMD bridges for AMD CPUs vs Intel bridges for Intel
  // CPUs). TODO FIXME This is not always true, but we don't have a better
  // logic for the time
  do {
    hwloc_obj_t bridge = NULL;
    while ((bridge = hwloc_get_next_bridge(pocl_topology, bridge))) {
      union hwloc_obj_attr_u *attr = bridge->attr;
      unsigned int vid;
      if (!attr)
	continue;
      vid = attr->bridge.upstream.pci.vendor_id;
      if (vid) {
	device->vendor_id = vid;
	break;
      }
    }
  } while (0);

  // Destroy topology object and return
  hwloc_topology_destroy(pocl_topology);

}


