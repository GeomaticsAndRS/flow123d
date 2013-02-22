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
 * @file    output.h
 * @brief   Header: The functions for all outputs.
 *
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <vector>
#include <string>
#include <fstream>
#include <mpi.h>

#include "system/xio.h"
#include "mesh/mesh.h"

#include "input/accessors.hh"

class OutputFormat;

/**
 * Class of output data storing reference on data.
 *
 * This class is referenced in Output and OuputTime class. The object contains
 * only reference on data. The referenced data could not be freed or rewrite
 * until data are written to the output file.
 */
class OutputData {
private:
public:
    // Types of data, that could be written to output file
    typedef enum {
        OUT_VECTOR_INT_SCA,
        OUT_VECTOR_INT_VEC,
        OUT_VECTOR_FLOAT_SCA,
        OUT_VECTOR_FLOAT_VEC,
        OUT_VECTOR_DOUBLE_SCA,
        OUT_VECTOR_DOUBLE_VEC,
        OUT_ARRAY_INT_SCA,
        OUT_ARRAY_FLOAT_SCA,
        OUT_ARRAY_DOUBLE_SCA
    } OutDataType;

    // Types of reference data
    typedef enum {
        NODE_DATA,
        CORNER_DATA,
        ELEM_DATA
    } RefType;

    string          *name;      ///< String with name of data
    string          *units;     ///< String with units
    void            *data;      ///< Pointer at own data
    OutDataType     type;       ///< Type values in vector
    RefType         ref_type;   ///< Type of reference data
    int             comp_num;   ///< Number of components in vector
    int             num;        ///< Number of values in vector/array


    OutputData() {};            ///< Un-named constructor can't be called directly

    string* getName(void) { return name; };
    string* getUnits(void) { return units; };
    int getCompNum(void) { return comp_num; };
    int getValueNum(void) { return num; };

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, int *data, unsigned int size);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, float *data, unsigned int size);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, double *data, unsigned int size);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, std::vector<int> &data);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, std::vector< vector<int> > &data);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, std::vector<float> &data);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, std::vector< vector<float> > &data);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, std::vector<double> &data);

    /**
     * \brief Constructor for OutputData storing names of output data and their
     * units.
     */
    OutputData(std::string name, std::string unit, std::vector< vector<double> > &data);

    /**
     * \brief Destructor for OutputData
     */
    ~OutputData();
};

/**
 * Definition of output data vector
 */
typedef std::vector<OutputData> OutputDataVec;

/**
 * Class of output. This class is used for output data to the output files.
 *
 * This class is used for writing static data to the output files. This class
 * itself doesn't include any method for writing data to the output file.
 * The methods for writing data to the output files are in places. Look at
 * output_vtk.cc and output_msh.cc that implement output for specific file
 * formats.
 */
class Output {
public:
    /*
     * \brief Temporary definition for storing data (C++ vector of double scalars)
     */
    typedef std::vector<double> ScalarFloatVector;

    /**
     * \brief Temporary definition for storing data (C++ vector of double vectors)
     */
    typedef std::vector< vector<double> > VectorFloatVector;

    /**
     * \brief Temporary structure for storing data (double scalars)
     */
    typedef struct OutScalar {
        ScalarFloatVector   *scalars;
        string              name;
        string              unit;
    } _OutScalar;

    /**
     * \brief Temporary structure for storing data (double vectors)
     */
    typedef struct OutVector {
        VectorFloatVector   *vectors;
        string              name;
        string              unit;
    } _OutVector;

    /**
     * \brief Temporary vectors of structures for storing data (double scalars)
     */
    typedef std::vector<OutScalar> OutScalarsVector;

    /**
     * \brief Temporary vectors of structures for storing data (double vectors)
     */
    typedef std::vector<OutVector> OutVectorsVector;

    /**
     * \brief Constructor of the Output object
     *
     * \param[in] mesh    The pointer at Mesh
     * \param[in] filename    The name of the output file
     */
    Output(Mesh *mesh, string filename);

    /**
     * \brief Destructor of the Output object.
     */
    virtual ~Output();

    /**
     * \brief Register array of data on nodes.
     *
     * This function will add reference on the array of data to the Output object.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in]   name    The name of data
     * \param[in]   unit    The name of units
     * \param[in]   data    The pointer on array of data
     * \param[in]   size    The number of values in array
     *
     * \return This function return 1, when the length of data vector is the same as
     * number of nodes in mesh. When the the number is different, then this
     * function returns 0.
     */
    template <typename _Data>
    int register_node_data(std::string name, std::string unit, _Data *data, uint size);

    /**
     * \brief Register array of data on elements.
     *
     * This function will add reference on this array of data to the Output object.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in]   name    The name of data
     * \param[in]   unit    The name of units
     * \param[in]   data    The pointer on array of data
     * \param[in]   size    The number of values in array
     *
     * \return This function return 1, when the length of data vector is the same as
     * number of elements in mesh. When the the number is different, then this
     * function returns 0.
     */
    template <typename _Data>
    int register_elem_data(std::string name, std::string unit, _Data *data, unsigned int size);

    /**
     * \brief Register data on nodes.
     *
     * This function will add reference on this data to the Output object. Own data
     * will be written to the file, when write_data() method will be called.
     *
     * \param[in]   name    The name of data
     * \param[in]   unit    The name of units
     * \param[in]   data    The reference on vector of data
     *
     * \return This function return 1, when the length of data vector is the same as
     * number of nodes in mesh. When the the number is different, then this function
     * returns 0.
     */
    template <typename _Data>
    int register_node_data(std::string name, std::string unit, std::vector<_Data> &data);

    /**
     * \brief Register data on elements.
     *
     * This function will add reference on the data to the Output object. Own data
     * will be written to the file, when write_data() method will be called.
     *
     * \param[in]   name    The name of data
     * \param[in]   unit    The name of units
     * \param[in]   data    The reference on vector of data
     *
     * \return This function return 1, when the length of data vector is the same as
     * number of elements in mesh. When the the number is different, then this
     * function returns 0.
     */
    template <typename _Data>
    int register_elem_data(std::string name, std::string unit, std::vector<_Data> &data);

    int write_head(void);

    int write_tail(void);

    /**
     * \brief This method is virtual method. Each file format should include
     * own method for writing data to output file.
     *
     * \return This function return result of pointer at output function.
     */
    int write_data(void);

    // TODO: Move public setters and getters to protected section
    std::vector<OutputData> *get_node_data(void) { return node_data; };
    std::vector<OutputData> *get_corner_data(void) { return corner_data; };
    std::vector<OutputData> *get_elem_data(void) { return elem_data; };
    ofstream& get_base_file(void) { return *base_file; };
    string& get_base_filename(void) { return *base_filename; };
    ofstream& get_data_file(void) { return *data_file; };
    string& get_data_filename(void) { return *data_filename; };
    Mesh *get_mesh(void) { return mesh; };
    unsigned int get_corner_count(void) { return this->corner_count; }
    void set_corner_count(unsigned int count) { this->corner_count = count; }

    void set_data_file(ofstream *_data_file) { data_file = _data_file; };

    typedef enum {
        NONE = 0,
        GMSH = 1,
        VTK = 2,
    } OutFileFormat;

    OutFileFormat file_format;

    OutputFormat *output_format;

    string          *name;              ///< Name of output stream

protected:
    // Protected setters for descendant
    void set_mesh(Mesh *_mesh) { mesh = _mesh; };
    void set_base_file(ofstream *_base_file) { base_file = _base_file; };
    void set_base_filename(string *_base_filename) { base_filename = _base_filename; };
    void set_node_data(std::vector<OutputData> *_node_data) { node_data = _node_data; };
    void set_corner_data(std::vector<OutputData> *_corner_data) { corner_data = _corner_data; };
    void set_elem_data(std::vector<OutputData> *_elem_data) { elem_data = _elem_data; };

    Output() { node_scalar = NULL; element_scalar = NULL; element_vector = NULL; };

private:

    struct OutScalar *node_scalar;      // Temporary solution
    struct OutScalar *element_scalar;   // Temporary solution
    struct OutVector *element_vector;   // Temporary solution

    ofstream        *base_file;         ///< Base output stream
    string          *base_filename;     ///< Name of base output file
    string          *data_filename;     ///< Name of data output file
    ofstream        *data_file;         ///< Data output stream (could be same as base_file)
    Mesh            *mesh;
    OutputDataVec   *node_data;         ///< The list of data on nodes
    OutputDataVec   *corner_data;       ///< The list of data on corners
    OutputDataVec   *elem_data;         ///< The list of data on elements

    /**
     * \brief The total number of corners of all elements
     */
    unsigned int corner_count;
};

template <typename _Data>
int Output::register_node_data(std::string name,
        std::string unit,
        _Data *data,
        uint size)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    if(mesh->node_vector.size() == size) {
        int found = 0;

        OutputData *out_data = new OutputData(name, unit, data, size);
        out_data->ref_type = OutputData::NODE_DATA;
        node_data->push_back(*out_data);

        return 1;
    } else {
        xprintf(Err, "Number of values: %d is not equal to number of nodes: %d\n", size, mesh->node_vector.size());
        return 0;
    }
}

template <typename _Data>
int Output::register_elem_data(std::string name,
        std::string unit,
        _Data *data,
        uint size)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    if(mesh->element.size() == size) {
        int found = 0;

        OutputData *out_data = new OutputData(name, unit, data, size);
        out_data->ref_type = OutputData::ELEM_DATA;
        elem_data->push_back(*out_data);

        return 1;
    } else {
        xprintf(Err, "Number of values: %d is not equal to number of elements: %d\n", size, mesh->element.size());
        return 0;
    }
}

template <typename _Data>
int Output::register_node_data(std::string name,
        std::string unit,
        std::vector<_Data> &data)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    if(mesh->node_vector.size() == data.size()) {
        OutputData *out_data = new OutputData(name, unit, data);
        out_data->ref_type = OutputData::NODE_DATA;
        node_data->push_back(*out_data);
        return 1;
    } else {
        xprintf(Err, "Number of values: %d is not equal to number of nodes: %d\n", data.size(), mesh->node_vector.size());
        return 0;
    }
}

template <typename _Data>
int Output::register_elem_data(std::string name,
        std::string unit,
        std::vector<_Data> &data)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    if(mesh->element.size() == data.size()) {
        OutputData *out_data = new OutputData(name, unit, data);
        out_data->ref_type = OutputData::ELEM_DATA;
        elem_data->push_back(*out_data);
        return 1;
    } else {
        xprintf(Err, "Number of values: %d is not equal to number of elements: %d\n", data.size(), mesh->element.size());
        return 0;
    }
}

/**
 * \brief The class for outputing data during time.
 *
 * This class is descendant of Output class. This class is used for outputing
 * data varying in time. Own output to specific file formats is done at other
 * places to. See output_vtk.cc and output_msh.cc.
 */
class OutputTime : public Output {
protected:
    OutputTime() {};
private:
public:
    /**
     * \brief Array of pointer at OutputTime
     */
    static OutputTime **output_streams;

    /**
     * \brief The count of instances
     */
    static int output_streams_count;

    /**
     * \brief Does OutputStream with same name and filename exist? When this
     * record is already created, then it returns pointer at this record. When
     * this record doesn' exixt, then it returns NULL pointer.
     *
     */
    static OutputTime *is_created(const Input::Record &in_rec);

    /**
     * \brief Constructor of OutputTime object. It opens base file for writing.
     *
     * \param[in]   mesh  The pointer at mesh object.
     * \param[in]   in_rec The reference on the input record
     */
    OutputTime(Mesh *mesh, const Input::Record &in_rec);

    /**
     * \brief Destructor of OutputTime. It doesn't do anything, because all
     * necessary destructors will be called in destructor of Output
     */
    virtual ~OutputTime();

    /**
     * \brief The specification of output stream
     *
     * \return This variable defines record for output stream
     */
    static Input::Type::Record input_type;


    /**
     * \brief This function register data on nodes.
     *
     * This function will add reference on this array of data to the Output object.
     * It is possible to call this function only once, when data are at the same
     * address during time. It is possible to call this function for each step, when
     * data are not at the same address, but name of the data has to be same.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in] name  The name of data
     * \param[in] unit  The units of data
     * \param[in] *data The pointer at data (array of int, float or double)
     * \param[in] size  The size of array (number of values)
     *
     * \return This function returns 1, when data were registered. This function
     * returns 0, when it wasn't able to register data (number of values isn't
     * same as number of nodes).
     */
    template <typename _Data>
    int register_node_data(std::string name, std::string unit, _Data *data, unsigned int size);

    /**
     * \brief This function register data on corners of triangles.
     *
     * This function will add reference on this array of data to the Output object.
     * It is possible to call this function only once, when data are at the same
     * address during time. It is possible to call this function for each step, when
     * data are not at the same address, but name of the data has to be same.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in] name  The name of data
     * \param[in] unit  The units of data
     * \param[in] *data The pointer at data (array of int, float or double)
     * \param[in] size  The size of array (number of values)
     *
     * \return This function returns 1, when data were registered. This function
     * returns 0, when it wasn't able to register data (number of values isn't
     * same as number of nodes).
     */
    template <typename _Data>
    int register_corner_data(std::string name, std::string unit, _Data *data, unsigned int size);

    /**
     * \brief This function register data on elements.
     *
     * This function will add reference on this array of data to the Output object.
     * It is possible to call this function only once, when data are at the same
     * address during time. it is possible to call this function for each step, when
     * data are not at the same address, but name of the data has to be same.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in] name  The name of data
     * \param[in] unit  The units of data
     * \param[in] *data The pointer at data (array of int, float or double)
     * \param[in] size  The size of array (number of values)
     *
     * \return This function returns 1, when data were registered. This function
     * returns 0, when it wasn't able to register data (number of values isn't
     * same as number of elements).
     */
    template <typename _Data>
    int register_elem_data(std::string name, std::string unit, _Data *data, unsigned int size);

    /**
     * \brief This function register data on nodes.
     *
     * This function will add reference on this array of data to the Output object.
     * It is possible to call this function only once, when data are at the same
     * address during time. it is possible to call this function for each step, when
     * data are not at the same address, but name of the data has to be same.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in] name  The name of data
     * \param[in] unit  The units of data
     * \param[in] *data The pointer at data (array of int, float or double)
     *
     * \return This function returns 1, when data were registered. This function
     * returns 0, when it wasn't able to register data (number of values isn't
     * same as number of nodes).
     */
    template <typename _Data>
    int register_node_data(std::string name, std::string unit, std::vector<_Data> &data);

    /**
     * \brief This function register data on corners of triangles.
     *
     * This function will add reference on this array of data to the Output object.
     * It is possible to call this function only once, when data are at the same
     * address during time. it is possible to call this function for each step, when
     * data are not at the same address, but name of the data has to be same.
     * Own data will be written to the file, when write_data() method will be called.
     *
     * \param[in] name  The name of data
     * \param[in] unit  The units of data
     * \param[in] *data The pointer at data (array of int, float or double)
     *
     * \return This function returns 1, when data were registered. This function
     * returns 0, when it wasn't able to register data (number of values isn't
     * same as number of nodes).
     */
    template <typename _Data>
    int register_corner_data(std::string name, std::string unit, std::vector<_Data> &data);

    /**
     * \brief Register vector of data on elements.
     *
     * This function will add reference on the data to the Output object. Own
     * data will be written to the file, when write_data() method will be called.
     * When the data has been already registered, then pointer at data will be
     * updated. Otherwise, new data will be registered.
     *
     * \param[in] name  The name of data
     * \param[in] unit  The unit of data
     * \param[in] &data The reference on vector (int, float, double)
     *
     * \return This function returns 1, when data were successfully registered.
     * This function returns 0, when number of elements and items of vector is
     * not the same.
     */
    template <typename _Data>
    int register_elem_data(std::string name, std::string unit, std::vector<_Data> &data);

    /**
     * \brief This is virtual method. Every file format should specify its own
     * method for writing data to output file.
     *
     * \param[in] time  The output will be done for this time
     *
     * \return This function returns result of method _write_data().
     */
    int write_data(double time);

    int              current_step;      ///< Current step

};

template <typename _Data>
int OutputTime::register_node_data(std::string name,
        std::string unit,
        _Data *data,
        uint size)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int found = 0;

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    std::vector<OutputData> *node_data = get_node_data();
    Mesh *mesh = get_mesh();

    ASSERT(mesh->node_vector.size() == size,
            "mesh->node_vector.size(): %d != size: %d",
            mesh->node_vector.size(),
            size);

    for(std::vector<OutputData>::iterator od_iter = node_data->begin();
            od_iter != node_data->end();
            od_iter++)
    {
        if(*od_iter->name == name) {
            od_iter->data = (void*)data;
            found = 1;
            break;
        }
    }

    if(found == 0) {
        OutputData *out_data = new OutputData(name, unit, data, size);
        out_data->ref_type = OutputData::NODE_DATA;
        node_data->push_back(*out_data);
    }

    return 1;

}

template <typename _Data>
int OutputTime::register_corner_data(std::string name,
        std::string unit,
        _Data *data,
        uint size)
{
    int rank=0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int found = 0;

    /* It's possible now to do output to the file only in the first process */
    if(rank!=0) {
        /* TODO: do something, when support for Parallel VTK is added */
        return 0;
    }

    /* TODO: It's not possible to check easily correct number of corners, so
     * skipping for now. */

    std::vector<OutputData> *corner_data = get_corner_data();
    Mesh *mesh = get_mesh();

    for(std::vector<OutputData>::iterator od_iter = corner_data->begin();
            od_iter != corner_data->end();
            od_iter++)
    {
        if(*od_iter->name == name) {
            od_iter->data = (void*)data;
            found = 1;
            break;
        }
    }

    if(found == 0) {
        OutputData *out_data = new OutputData(name, unit, data, size);
        out_data->ref_type = OutputData::CORNER_DATA;
        corner_data->push_back(*out_data);
    }

    return 1;

}

template <typename _Data>
int OutputTime::register_elem_data(std::string name,
        std::string unit,
        _Data *data,
        unsigned int size)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int found = 0;

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    std::vector<OutputData> *elem_data = get_elem_data();
    Mesh *mesh = get_mesh();

    ASSERT(mesh->element.size() == size,
            "mesh->element.size(): %d != size: %d",
            mesh->element.size(),
            size);

    for(std::vector<OutputData>::iterator od_iter = elem_data->begin();
            od_iter != elem_data->end();
            od_iter++)
    {
        if(*od_iter->name == name) {
            od_iter->data = (void*)data;
            found = 1;
            break;
        }
    }

    if(found == 0) {
        OutputData *out_data = new OutputData(name, unit, data, size);
        out_data->ref_type = OutputData::ELEM_DATA;
        elem_data->push_back(*out_data);
    }

    return 1;
}

template <typename _Data>
int OutputTime::register_node_data(std::string name,
        std::string unit,
        std::vector<_Data> &data)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int found = 0;

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    std::vector<OutputData> *node_data = get_node_data();
    Mesh *mesh = get_mesh();

    ASSERT(mesh->node_vector.size() == data.size(),
            "mesh->node_vector.size(): %d != size: %d",
            mesh->node_vector.size(),
            data.size());

    for(std::vector<OutputData>::iterator od_iter = node_data->begin();
            od_iter != node_data->end();
            od_iter++)
    {
        if(*od_iter->name == name) {
            od_iter->data = (void*)&data;
            found = 1;
            break;
        }
    }

    if(found == 0) {
        OutputData *out_data = new OutputData(name, unit, data);
        out_data->ref_type = OutputData::NODE_DATA;
        node_data->push_back(*out_data);
    }

    return 1;
}

template <typename _Data>
int OutputTime::register_corner_data(std::string name,
        std::string unit,
        std::vector<_Data> &data)
{
    int rank=0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int found = 0;

    /* It's possible now to do output to the file only in the first process */
    if(rank!=0) {
        /* TODO: do something, when support for Parallel VTK is added */
        return 0;
    }

    /* TODO: It's not possible to check easily correct number of corners, so
     * skipping for now. */

    std::vector<OutputData> *corner_data = get_corner_data();
    Mesh *mesh = get_mesh();

    for(std::vector<OutputData>::iterator od_iter = corner_data->begin();
            od_iter != corner_data->end();
            od_iter++)
    {
        if(*od_iter->name == name) {
            od_iter->data = (void*)&data;
            found = 1;
            break;
        }
    }

    if(found == 0) {
        OutputData *out_data = new OutputData(name, unit, data);
        out_data->ref_type = OutputData::CORNER_DATA;
        corner_data->push_back(*out_data);
    }

    return 1;
}

template <typename _Data>
int OutputTime::register_elem_data(std::string name,
        std::string unit,
        std::vector<_Data> &data)
{
	int rank=0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int found = 0;

	/* It's possible now to do output to the file only in the first process */
	if(rank!=0) {
		/* TODO: do something, when support for Parallel VTK is added */
		return 0;
	}

    std::vector<OutputData> *elem_data = get_elem_data();
    Mesh *mesh = get_mesh();

    ASSERT(mesh->element.size() == data.size(),
            "mesh->element.size(): %d != size: %d",
            mesh->element.size(),
            data.size());

    for(std::vector<OutputData>::iterator od_iter = elem_data->begin();
            od_iter != elem_data->end();
            od_iter++)
    {
        if(*od_iter->name == name) {
            od_iter->data = (void*)&data;
            found = 1;
            break;
        }
    }

    if(found == 0) {
        OutputData *out_data = new OutputData(name, unit, data);
        out_data->ref_type = OutputData::ELEM_DATA;
        elem_data->push_back(*out_data);
    }
    return 1;

}

/**
 * \brief The class used as parent of class of file formats
 */
class OutputFormat {
public:
	OutputFormat() {}
    virtual ~OutputFormat() {}
	virtual int write_data(void) { return 0; }
	virtual int write_data(double time) { return 0; }
	virtual int write_head(void) { return 0; }
	virtual int write_tail(void) { return 0; }

	static Input::Type::AbstractRecord input_type;
};

inline OutputTime *OutputStream(Mesh *mesh, const Input::Record &in_rec)
{
    int rank=0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0) {
        return NULL;
    }

    OutputTime *output_time = OutputTime::is_created(in_rec);

    /* When this record doesn't exist, then create new one */
    if(output_time == NULL) {
        /* Realloc array of output_streams */
        OutputTime **new_ptr = (OutputTime**)xrealloc(OutputTime::output_streams,
                sizeof(void*) * (OutputTime::output_streams_count + 1));
        OutputTime::output_streams = new_ptr;
        /* Create new output */
        output_time = new OutputTime(mesh, in_rec);
        /* Add this output to the array of output streams */
        OutputTime::output_streams[OutputTime::output_streams_count-1] = output_time;
    }

    return output_time;
}

#endif
