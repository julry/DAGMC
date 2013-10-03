// MCNP5/dagmc/MeshTally.cpp

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>

#include "moab/Interface.hpp"

#include "MeshTally.hpp" 

//---------------------------------------------------------------------------//
// CONSTRUCTOR
//---------------------------------------------------------------------------//
MeshTally::MeshTally(const TallyInput& input)
	: Tally(input)    
{
    // Determine name of the output file
    TallyInput::TallyOptions::iterator it = input_data.options.find("out");

    if (it != input_data.options.end())
    {
        output_filename = it->second;
        input_data.options.erase(it);
    }
    else // use default output file name
    {
        std::stringstream str;
        str << "meshtal" << input.tally_id << ".h5m";
        str >> output_filename;
    }

    // Reset the iterator and find the name of the input mesh file
    it = input_data.options.begin();
    it = input_data.options.find("inp");

    if (it != input_data.options.end())
    {
 	input_filename = it->second;
        input_data.options.erase(it);
    }
    else
    {
       std::cerr << "Exit: No input mesh file was given." << std::endl;
       exit(EXIT_FAILURE); 
    }
}
//---------------------------------------------------------------------------//
// PUBLIC INTERFACE
//---------------------------------------------------------------------------//
/*
void MeshTally::end_history()
{
    std::set<moab::EntityHandle>::iterator i;

    // add sum of scores for this history to mesh tally for each tally point
    for (i = visited_this_history.begin(); i != visited_this_history.end(); ++i)
    {
        for (unsigned int j = 0; j < data->get_num_energy_bins(); ++j)
        {
            double& history_score = get_data(temp_tally_data, *i, j);
            double& tally = get_data(tally_data, *i, j);
            double& error = get_data(error_data, *i, j);

            tally += history_score;
            error += history_score * history_score;
      
            // reset temp_tally_data array for the next particle history
            history_score = 0;
        }
    }

    // reset set of tally points for next particle history
    visited_this_history.clear();
}
*/
//---------------------------------------------------------------------------//
// PROTECTED METHODS
//---------------------------------------------------------------------------//
/*
void MeshTally::add_score_to_tally(moab::EntityHandle tally_point,
                                   double score,
                                   int ebin)
{
    // update tally for this history with new score
    get_data(temp_tally_data, tally_point, ebin) += score;

    // also update total energy bin tally for this history if one exists
    if (data->has_total_energy_bin())
    {
        get_data(temp_tally_data, tally_point, (data->get_num_energy_bins()-1)) += score;
    }

    visited_this_history.insert(tally_point);
}
*/
//---------------------------------------------------------------------------//
unsigned int MeshTally::get_entity_index(moab::EntityHandle tally_point)
{
    unsigned int ret = tally_points.index(tally_point);
    assert(ret < tally_points.size());
    return ret;
}
//---------------------------------------------------------------------------//
moab::ErrorCode MeshTally::load_moab_mesh(moab::Interface* mbi,
                                          moab::EntityHandle& mesh_set)
{
    // create a mesh set to store the MOAB mesh data
    moab::ErrorCode rval = mbi->create_meshset(moab::MESHSET_SET, mesh_set);
    
    if (rval != moab::MB_SUCCESS) return rval;

    // load the MOAB mesh data from the input file into the mesh set
    rval = mbi->load_file(input_filename.c_str(), &mesh_set);

    if (rval != moab::MB_SUCCESS) return rval;

    return moab::MB_SUCCESS;
}
//---------------------------------------------------------------------------//
void MeshTally::set_tally_points(const moab::Range& mesh_elements)
{
    tally_points = mesh_elements;

    // resize data arrays for storing the tally data for these tally points
    data->resize_data_arrays(tally_points.size());

    // measure number of divisions in moab::Range representing tally points
    int psize = tally_points.psize();

    std::cout << "    Tally range has psize: " << psize << std::endl;

    // print warning about performance compromise if there are many divisions
    // (for some rather arbitrary definition of "many")
    if (psize > 4)
    {
        std::cerr << "Warning: large tally range psize " << psize
                  << ", may reduce performance" << std::endl;
    }
}
//---------------------------------------------------------------------------//
moab::ErrorCode MeshTally::reduce_meshset_to_3D(moab::Interface* mbi,
                                                moab::EntityHandle& mesh_set,
                                                moab::Range& mesh_elements)
{
    // get all 3D elements from the mesh set
    moab::ErrorCode rval;
    rval = mbi->get_entities_by_dimension(mesh_set, 3, mesh_elements);

    if (rval != moab::MB_SUCCESS) return rval;

    // clear the mesh set and add 3D elements
    rval = mbi->clear_meshset(&mesh_set, 1);

    if (rval != moab::MB_SUCCESS) return rval;

    rval = mbi->add_entities(mesh_set, mesh_elements);

    if (rval != moab::MB_SUCCESS) return rval;

    return moab::MB_SUCCESS;
}
//---------------------------------------------------------------------------//
moab::ErrorCode MeshTally::setup_tags(moab::Interface* mbi, const char* prefix)
{ 
    moab::ErrorCode rval;
    std::string pfx = prefix;
    int tag_size = 1;
    
    unsigned int num_bins = data->get_num_energy_bins();
    tally_tags.resize(num_bins);
    error_tags.resize(num_bins);

    // Create separate MOAB tag handles for every energy bin
    for(unsigned i = 0; i < num_bins; ++i)
    {
        std::string t_name = pfx + "TALLY_TAG", e_name = pfx + "ERROR_TAG";
        std::stringstream str;  

        if(i + 1 != num_bins)
        {
            str << "_" << input_data.energy_bin_bounds[i] 
                << '-' << input_data.energy_bin_bounds[i+1];
        }
  
        t_name += str.str();
        e_name += str.str();

        rval = mbi->tag_get_handle(t_name.c_str(),
                                   tag_size,
                                   moab::MB_TYPE_DOUBLE, 
                                   tally_tags[i],
                                   moab::MB_TAG_DENSE|moab::MB_TAG_CREAT);

        if(rval != moab::MB_SUCCESS) return rval;
    
        rval = mbi->tag_get_handle(e_name.c_str(),
                                   tag_size,
                                   moab::MB_TYPE_DOUBLE,
                                   error_tags[i],
                                   moab::MB_TAG_DENSE|moab::MB_TAG_CREAT);

        if(rval != moab::MB_SUCCESS) return rval;
    }

    return moab::MB_SUCCESS;
}
//---------------------------------------------------------------------------//

// end of MCNP5/dagmc/MeshTally.cpp
