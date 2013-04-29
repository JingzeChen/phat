/*  Copyright 2013 IST Austria
    Contributed by: Ulrich Bauer, Michael Kerber, Jan Reininghaus

    This file is part of PHAT.

    PHAT is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PHAT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with PHAT.  If not, see <http://www.gnu.org/licenses/>. */

#include <phat/compute_persistence_pairs.h>

#include <phat/representations/vector_vector.h>
#include <phat/representations/vector_set.h>
#include <phat/representations/vector_list.h>
#include <phat/representations/sparse_pivot_column.h>
#include <phat/representations/full_pivot_column.h>
#include <phat/representations/bit_tree_pivot_column.h>

#include <phat/algorithms/twist_reduction.h>
#include <phat/algorithms/standard_reduction.h>
#include <phat/algorithms/row_reduction.h>
#include <phat/algorithms/chunk_reduction.h>

#include <iostream>
#include <iomanip>


enum Representation_type  {VECTOR_VECTOR, VECTOR_SET, SPARSE_PIVOT_COLUMN, FULL_PIVOT_COLUMN, BIT_TREE_PIVOT_COLUMN, VECTOR_LIST};
enum Algorithm_type  {STANDARD, TWIST, ROW, CHUNK};
enum Ansatz_type  {PRIMAL, DUAL};

void print_help() {
    std::cerr << "Usage: " << "benchmark " << "[options] input_filename_0 input_filename_1 ... input_filename_N" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "--ascii   --  use ascii file format" << std::endl;
    std::cerr << "--binary  --  use binary file format (default)" << std::endl;
    std::cerr << "--help    --  prints this screen" << std::endl;
    std::cerr << "--dualize   --  use only dualization approach" << std::endl;
    std::cerr << "--primal   --  use only primal approach" << std::endl;
    std::cerr << "--vector_vector, --vector_set, --vector_list, --full_pivot_column, --sparse_pivot_column, --bit_tree_pivot_column  --  use only a subset of representation data structures for boundary matrices" << std::endl;
    std::cerr << "--standard, --twist, --chunk, --row  --  use only a subset of reduction algorithms" << std::endl;
}

void print_help_and_exit() {
    print_help();
    exit( EXIT_FAILURE );
}

void parse_command_line( int argc, char** argv, bool& use_binary, std::vector< Representation_type >& representations, std::vector< Algorithm_type >& algorithms
                       , std::vector< Ansatz_type >& ansaetze, std::vector< std::string >& input_filenames ) {

    if( argc < 2 ) print_help_and_exit();

    int number_of_options = 0;
    for( int idx = 1; idx < argc; idx++ ) {
        const std::string argument = argv[ idx ];
        if( argument.size() > 2 && argument[ 0 ] == '-' && argument[ 1 ] == '-' ) {
            if( argument == "--ascii" ) use_binary = false;
            else if( argument == "--binary" ) use_binary = true;
            else if( argument == "--vector_vector" ) representations.push_back( VECTOR_VECTOR );
            else if( argument == "--vector_set" ) representations.push_back( VECTOR_SET );
            else if( argument == "--vector_list" ) representations.push_back( VECTOR_LIST );
            else if( argument == "--full_pivot_column" )  representations.push_back( FULL_PIVOT_COLUMN );
            else if( argument == "--bit_tree_pivot_column" )  representations.push_back( BIT_TREE_PIVOT_COLUMN );
            else if( argument == "--sparse_pivot_column" ) representations.push_back( SPARSE_PIVOT_COLUMN );
            else if( argument == "--standard" ) algorithms.push_back( STANDARD );
            else if( argument == "--twist" ) algorithms.push_back( TWIST );
            else if( argument == "--row" ) algorithms.push_back( ROW );
            else if( argument == "--chunk" ) algorithms.push_back( CHUNK );
            else if( argument == "--primal" ) ansaetze.push_back( PRIMAL );
            else if( argument == "--dual" ) ansaetze.push_back( DUAL );
            else if( argument == "--help" ) print_help_and_exit();
            else print_help_and_exit();
        } else {
            input_filenames.push_back( argument );
        }
    }

    if( representations.empty() == true ) {
        representations.push_back( VECTOR_VECTOR );
        representations.push_back( VECTOR_SET );
        representations.push_back( VECTOR_LIST );
        representations.push_back( FULL_PIVOT_COLUMN );
        representations.push_back( BIT_TREE_PIVOT_COLUMN );
        representations.push_back( SPARSE_PIVOT_COLUMN );
    }

    if( algorithms.empty() == true ) {
        algorithms.push_back( STANDARD );
        algorithms.push_back( TWIST );
        algorithms.push_back( ROW );
        algorithms.push_back( CHUNK );
    }
    
    if( ansaetze.empty() == true ) {
        ansaetze.push_back( PRIMAL );
        ansaetze.push_back( DUAL );
    }
}

template<typename Representation, typename Algorithm>
void compute( std::string input_filename,
              bool use_binary,
              Ansatz_type ansatz ) {

    phat::boundary_matrix< Representation > matrix;
    bool read_successful = use_binary ? matrix.load_binary( input_filename ) : matrix.load_ascii( input_filename );
   
    if( !read_successful ) {
        std::cerr << std::endl << " Error opening file " << input_filename << std::endl;
        print_help_and_exit();
    }

    double pairs_timer = omp_get_wtime();
    phat::persistence_pairs pairs;
    if( ansatz == PRIMAL ) {
        std::cout << " primal";
        phat::compute_persistence_pairs< Algorithm > ( pairs, matrix );
    } else {
        std::cout << " dual";
        phat::compute_persistence_pairs_dualized< Algorithm > ( pairs, matrix );
    }
    std::cout << " " << setiosflags( std::ios::fixed ) << setiosflags( std::ios::showpoint ) << std::setprecision( 1 ) << omp_get_wtime() - pairs_timer <<"s" << std::endl;
}


void benchmark( std::string input_filename, bool use_binary, Algorithm_type algorithm, Representation_type representation, Ansatz_type ansatz ) {
    std::cout << input_filename;
    
    switch( representation ) {
    case VECTOR_VECTOR:
        std::cout << " vector_vector";
        switch( algorithm ) {
        case STANDARD: std::cout << " standard_reduction"; compute< phat::vector_vector, phat::standard_reduction >( input_filename, use_binary, ansatz ); break;
        case TWIST: std::cout << " twist_reduction"; compute< phat::vector_vector, phat::twist_reduction >( input_filename, use_binary, ansatz ); break;
        case ROW: std::cout << " row_reduction"; compute< phat::vector_vector, phat::row_reduction >( input_filename, use_binary, ansatz ); break;
        case CHUNK: std::cout << " chunk_reduction"; compute< phat::vector_vector, phat::chunk_reduction >( input_filename, use_binary, ansatz ); break;
        } break;

    case VECTOR_SET:   
        std::cout << " vector_set";
        switch( algorithm ) {
        case STANDARD: std::cout << " standard_reduction"; compute< phat::vector_set, phat::standard_reduction >( input_filename, use_binary, ansatz ); break;
        case TWIST: std::cout << " twist_reduction"; compute< phat::vector_set, phat::twist_reduction >( input_filename, use_binary, ansatz ); break;
        case ROW: std::cout << " row_reduction"; compute< phat::vector_set, phat::row_reduction >( input_filename, use_binary, ansatz ); break;
        case CHUNK: std::cout << " chunk_reduction"; compute< phat::vector_set, phat::chunk_reduction >( input_filename, use_binary, ansatz ); break;
        } break;
    
    case VECTOR_LIST:
        std::cout << " vector_list";
        switch( algorithm ) {
        case STANDARD: std::cout << " standard_reduction"; compute< phat::vector_list, phat::standard_reduction >( input_filename, use_binary, ansatz ); break;
        case TWIST: std::cout << " twist_reduction"; compute< phat::vector_list, phat::twist_reduction >( input_filename, use_binary, ansatz ); break;
        case ROW: std::cout << " row_reduction"; compute< phat::vector_list, phat::row_reduction >( input_filename, use_binary, ansatz ); break;
        case CHUNK: std::cout << " chunk_reduction"; compute< phat::vector_list, phat::chunk_reduction >( input_filename, use_binary, ansatz ); break;
        } break;

    case FULL_PIVOT_COLUMN:
        std::cout << " full_pivot_column";
        switch( algorithm ) {
        case STANDARD: std::cout << " standard_reduction"; compute< phat::full_pivot_column, phat::standard_reduction >( input_filename, use_binary, ansatz ); break;
        case TWIST: std::cout << " twist_reduction"; compute< phat::full_pivot_column, phat::twist_reduction >( input_filename, use_binary, ansatz ); break;
        case ROW: std::cout << " row_reduction"; compute< phat::full_pivot_column, phat::row_reduction >( input_filename, use_binary, ansatz ); break;
        case CHUNK: std::cout << " chunk_reduction"; compute< phat::full_pivot_column, phat::chunk_reduction >( input_filename, use_binary, ansatz ); break;
        } break;

    case BIT_TREE_PIVOT_COLUMN:
        std::cout << " bit_tree_pivot_column";
        switch( algorithm ) {
        case STANDARD: std::cout << " standard_reduction"; compute< phat::bit_tree_pivot_column, phat::standard_reduction >( input_filename, use_binary, ansatz ); break;
        case TWIST: std::cout << " twist_reduction"; compute< phat::bit_tree_pivot_column, phat::twist_reduction >( input_filename, use_binary, ansatz ); break;
        case ROW: std::cout << " row_reduction"; compute< phat::bit_tree_pivot_column, phat::row_reduction >( input_filename, use_binary, ansatz ); break;
        case CHUNK: std::cout << " chunk_reduction"; compute< phat::bit_tree_pivot_column, phat::chunk_reduction >( input_filename, use_binary, ansatz ); break;
        } break;

    case SPARSE_PIVOT_COLUMN:
        std::cout << " sparse_pivot_column";
        switch( algorithm ) {
        case STANDARD: std::cout << " standard_reduction"; compute< phat::sparse_pivot_column, phat::standard_reduction >( input_filename, use_binary, ansatz ); break;
        case TWIST: std::cout << " twist_reduction"; compute< phat::sparse_pivot_column, phat::twist_reduction >( input_filename, use_binary, ansatz ); break;
        case ROW: std::cout << " row_reduction"; compute< phat::sparse_pivot_column, phat::row_reduction >( input_filename, use_binary, ansatz ); break;
        case CHUNK: std::cout << " chunk_reduction"; compute< phat::sparse_pivot_column, phat::chunk_reduction >( input_filename, use_binary, ansatz ); break;
        } break;
    }
}

int main( int argc, char** argv )
{
    bool use_binary = true; // interpret inputs as binary or ascii files
    std::vector< std::string > input_filenames; // name of file that contains the boundary matrix

    std::vector< Representation_type > representations; // representation class
    std::vector< Algorithm_type > algorithms; // reduction algorithm
    std::vector< Ansatz_type > ansaetze; // primal / dual

    parse_command_line( argc, argv, use_binary, representations, algorithms, ansaetze, input_filenames );

    for( int idx_input = 0; idx_input < input_filenames.size(); idx_input++ ) {
        std::string input_filename = input_filenames[ idx_input ];
        for( int idx_algorithm = 0; idx_algorithm < algorithms.size(); idx_algorithm++ ) {
            Algorithm_type algorithm = algorithms[ idx_algorithm ];
            for( int idx_representation = 0; idx_representation < representations.size(); idx_representation++ ) {
                Representation_type representation = representations[ idx_representation ];
                for( int idx_ansatz = 0; idx_ansatz < ansaetze.size(); idx_ansatz++ ) {
                    Ansatz_type ansatz = ansaetze[ idx_ansatz ];
                    benchmark( input_filename, use_binary, algorithm, representation, ansatz );
                }
            }
        }
    }
}
