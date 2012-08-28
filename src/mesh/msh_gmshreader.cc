/*!
 *
 * Copyright (C) 2007 Technical University of Liberec.  All rights reserved.
 *
 * Please make a following refer to Flow123d on your project site if you use the program for any purpose,
 * especially for academic research:
 * Flow123d, Research Centre: Advanced Remedial Technologies, Technical University of Liberec, Czech Republic
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 021110-1307, USA.
 *
 *
 * $Id$
 * $Revision$
 * $LastChangedBy$
 * $LastChangedDate$
 *
 * @file
 * @ingroup mesh
 * @brief
 * @author dalibor
 * 
 * @date Created on October 3, 2010, 11:32 AM
 */

#include "msh_gmshreader.h"
#include "mesh/nodes.hh"
#include "system/xio.h"

GmshMeshReader::GmshMeshReader() {
    xprintf(Msg, " - GmshMeshReader()\n");
}

GmshMeshReader::~GmshMeshReader() {
}

/**
 *  Read mesh from file
 */
void GmshMeshReader::read(const std::string &fileName, Mesh* mesh) {
    xprintf(Msg, " - GmshMeshReader->read(const char* fileName, Mesh* mesh)\n");

    ASSERT(!(mesh == NULL), "Argument mesh is NULL in method GmshMeshRedaer->read(const char*, Mesh*)\n");

    FILE* file = xfopen(fileName.c_str(), "rt");

    read_nodes(file, mesh);
    read_elements(file, mesh);

    xfclose(file);

    mesh->setup_topology();
}

/**
 * private method for reading of nodes
 */
void GmshMeshReader::read_nodes(FILE* in, Mesh* mesh) {
    xprintf(Msg, " - Reading nodes...");

    char line[ LINE_SIZE ];

    skip_to(in, "$Nodes");
    xfgets(line, (LINE_SIZE - 2), in);
    int numNodes = atoi(xstrtok(line));
    ASSERT(!(numNodes < 1), "Number of nodes < 1 in function read_node_list()\n");

    mesh->node_vector.reserve(numNodes);

    for (int i = 0; i < numNodes; ++i) {
        xfgets(line, LINE_SIZE - 2, in);

        int id = atoi(xstrtok(line));
        //TODO: co kdyz id <= 0 ???
        INPUT_CHECK(!(id < 0), "Id of node must be > 0\n");


        double x = atof(xstrtok(NULL));
        double y = atof(xstrtok(NULL));
        double z = atof(xstrtok(NULL));

        NodeFullIter node = mesh->node_vector.add_item(id);
        //node->id = id;
        node->point()(0)=x;
        node->point()(1)=y;
        node->point()(2)=z;
    }

    //xprintf(MsgVerb, " %d nodes readed. ", nodeList->size());
    xprintf(Msg, " %d nodes readed. ", mesh->node_vector.size());
    xprintf(Msg, "O.K.\n");
}

/**
 * PARSE ELEMENT LINE
 */
void GmshMeshReader::parse_element_line(ElementVector &ele_vec, char *line, Mesh* mesh) {
    int id, ti, ni;
    int type;
    int n_tags = NDEF;

    F_ENTRY;
    ASSERT(NONULL(line), "NULL as argument of function parse_element_line()\n");

    //get element ID
    id = atoi(xstrtok(line));
    INPUT_CHECK(id >= 0, "Id number of element must be >= 0\n");

    //DBGMSG("add el: %d", id);
    ElementFullIter ele(ele_vec.add_item(id));

    //get element type: supported:
    //	1 Line (2 nodes)
    //	2 Triangle (3 nodes)
    //	4 Tetrahedron (4 nodes)
    type = atoi(xstrtok(NULL));
    switch (type) {
        case 1:
            ele->dim_ = 1;
            break;
        case 2:
            ele->dim_ = 2;
            break;
        case 4:
            ele->dim_ = 3;
            break;
        default:
            xprintf(UsrErr, "Element %d is of the unsupported type %d\n", ele.id(), type);
    }

    //get number of tags (at least 2)
    n_tags = atoi(xstrtok(NULL));
    INPUT_CHECK(!(n_tags < 2), "At least two element tags have to be defined. Elm %d\n", id);
    //get tags 1 and 2
    ele->mid = atoi(xstrtok(NULL));
    int rid = atoi(xstrtok(NULL));
    //get remaining tags
    if (n_tags > 2)  ele->pid = atoi(xstrtok(NULL)); // chop partition number in the new GMSH format
    for (ti = 3; ti < n_tags; ti++) xstrtok(NULL);         //skip remaining tags

    // allocate element arrays
    ele->node = new Node * [ele->n_nodes()];
    ele->edges_ = new Edge * [ele->n_sides()];
    ele->boundaries_ = new Boundary * [ele->n_sides()];
    ele->mesh_ = mesh;

    FOR_ELEMENT_SIDES(ele, si) {
        ele->edges_[ si ]=NULL;
        ele->boundaries_[si] =NULL;
    }

    FOR_ELEMENT_NODES(ele, ni) {
        int nodeId = atoi(xstrtok(NULL));
        NodeIter node = mesh->node_vector.find_id( nodeId );

        ASSERT(NONULL(node), "Unknown node with label %d\n", nodeId);

        ele->node[ni] = node;
    }
}


/**
 * private method for reading of elements - in process of implementation
 */
void GmshMeshReader::read_elements(FILE* in, Mesh * mesh) {
    xprintf(Msg, " - Reading elements...");

    char line[ LINE_SIZE ];

    skip_to(in, "$Elements");
    xfgets(line, LINE_SIZE - 2, in);
    int numElements = atoi(xstrtok(line));
    ASSERT(!(numElements < 1), "Number of elements < 1 in function read_node_list()\n");

    mesh->element.reserve(numElements);
    //init_element_list( mesh );

    for (int i = 0; i < numElements; ++i) {
        xfgets(line, LINE_SIZE - 2, in);
        parse_element_line(mesh->element, line, mesh);
    }
    ASSERT((mesh->n_elements() == numElements),
            "Number of created elements %d does not match number of elements %d in the input file.\n",
            mesh->n_elements(),
            numElements);

    xprintf(Msg, " %d elements readed. ", mesh->n_elements())/*orig verb 4*/;
    xprintf(Msg, "O.K.\n")/*orig verb 2*/;
}
