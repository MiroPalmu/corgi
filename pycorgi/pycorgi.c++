#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include <tuple>

#include "corgi/common.h"
#include "corgi/corgi.h"



PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>, true)


template<size_t D>
auto declare_tile(
    py::module &m, 
    const std::string& pyclass_name) 
{

    py::class_<corgi::Tile<D>, 
              std::shared_ptr<corgi::Tile<D> >
              > corgi_tile(m, pyclass_name.c_str());

    corgi_tile
        .def(py::init<>())

        .def_readwrite("cid",           &corgi::Tile<D>::cid)
        .def_readwrite("communication", &corgi::Tile<D>::communication)
        .def_readwrite("mins",          &corgi::Tile<D>::mins)
        .def_readwrite("maxs",          &corgi::Tile<D>::maxs)
        .def_readwrite("index",         &corgi::Tile<D>::index)
        .def_readwrite("lengths",       &corgi::Tile<D>::lengths)
        .def("get_index",               [](
              corgi::Tile<D>& t, corgi::Grid<D>& g)
            {
              corgi::internals::tuple_of<D, size_t> ind = 
                g.id2index( t.cid, g.lens() );

              return ind;
            })
        .def("set_tile_mins",           &corgi::Tile<D>::set_tile_mins)
        .def("set_tile_maxs",           &corgi::Tile<D>::set_tile_maxs)
        .def("load_metainfo",           &corgi::Tile<D>::load_metainfo)
        .def("nhood",                   &corgi::Tile<D>::nhood);

    return corgi_tile;
}


template<size_t D>
auto declare_node(
    py::module &m, 
    const std::string& pyclass_name) 
{

  
  // standard version with pybind default holder type 
  //py::class_<corgi::Grid<D> > corgi_node(m, pyclass_name.c_str());
      
  // Special version that makes grid indestructible from python 
  // (avoids py garbage collection)
  //
  // NOTE: this is dangerous because it can very easily lead to mem leaks;
  // we however assume that user only uses one grid per one run (because mpi 
  // also prevents multiple simultaneous versions). This *should* make it ok.
  // Without this both c++ grid and python call garbage collection on dangling 
  // tiles. This leads to double freeing of tiles and subsequent seg faulting.
  py::class_<corgi::Grid<D>,
      std::unique_ptr<corgi::Grid<D>, py::nodelete>
      > corgi_node(m, pyclass_name.c_str());

    corgi_node
        .def("rank",      [](corgi::Grid<D>& n) { return n.comm.rank(); })
        .def("size",      [](corgi::Grid<D>& n) { return n.comm.size(); })
        .def("master",    [](corgi::Grid<D>& n) { return n.comm.rank() == 0; })
        // specialized for dimensions
        //.def("add_tile",              &corgi::Grid<D>::add_tile, py::keep_alive<1,2>())
        .def("replace_tile",          &corgi::Grid<D>::replace_tile, py::keep_alive<1,2>())
        .def("get_tile_ids",           &corgi::Grid<D>::get_tile_ids,
                py::arg("sorted") = true)
        .def("get_tile", 
            (std::shared_ptr<corgi::Tile<D>> (corgi::Grid<D>::*)(const uint64_t)) 
              &corgi::Grid<D>::get_tileptr,
              py::return_value_policy::reference,
              py::keep_alive<1,0>()
              )

        .def("get_local_tiles",             &corgi::Grid<D>::get_local_tiles,
                 py::arg("sorted") = true)
        .def("get_virtual_tiles",          &corgi::Grid<D>::get_virtuals,
                 py::arg("sorted") = true)
        .def("get_boundary_tiles",          &corgi::Grid<D>::get_boundary_tiles,
                 py::arg("sorted") = true)

        .def("is_local",              &corgi::Grid<D>::is_local)
        .def("analyze_boundaries", &corgi::Grid<D>::analyze_boundaries)

        // // communication wrappers
        .def("send_tile",               &corgi::Grid<D>::send_tile)
        .def("recv_tile",               &corgi::Grid<D>::recv_tile)
        .def_readwrite("send_queue",         &corgi::Grid<D>::send_queue)
        .def_readwrite("send_queue_address", &corgi::Grid<D>::send_queue_address)
        .def("bcast_mpi_grid",          &corgi::Grid<D>::bcast_mpi_grid)
        .def("allgather_work_grid",     &corgi::Grid<D>::allgather_work_grid)
        .def("update_work",             &corgi::Grid<D>::update_work)

        .def("send_tiles",              &corgi::Grid<D>::send_tiles)
        .def("recv_tiles",              &corgi::Grid<D>::recv_tiles)
        .def("send_data",               &corgi::Grid<D>::send_data)
        .def("recv_data",               &corgi::Grid<D>::recv_data)
        .def("wait_data",               &corgi::Grid<D>::wait_data)

        // adoption routines
        .def("adopt",                   &corgi::Grid<D>::adopt)
        .def("adoption_council",        &corgi::Grid<D>::adoption_council)
        .def("adoption_council2",       &corgi::Grid<D>::adoption_council2)
        .def("communicate_adoptions",   &corgi::Grid<D>::communicate_adoptions)
        .def("erase_virtuals",          &corgi::Grid<D>::erase_virtuals);


  return corgi_node;
}




// --------------------------------------------------
PYBIND11_MODULE(pycorgi, m_base) {

    //--------------------------------------------------
    // dimension independent bindings

    py::class_<corgi::Communication> corgi_comm(m_base, "Communication");
    corgi_comm
        .def_readwrite("cid",                         &corgi::Communication::cid                        )
        .def_readwrite("indices",                     &corgi::Communication::indices                    )
        .def_readwrite("owner",                       &corgi::Communication::owner                      )
        .def_readwrite("top_virtual_owner",           &corgi::Communication::top_virtual_owner          )
        .def_readwrite("communications",              &corgi::Communication::communications             ) 
        .def_readwrite("number_of_virtual_neighbors", &corgi::Communication::number_of_virtual_neighbors)
        .def_readwrite("mins",                        &corgi::Communication::mins                       )
        .def_readwrite("maxs",                        &corgi::Communication::maxs                       );
        //.def_readwrite("local",                       &corgi::Communication::local                      )
        //.def_readwrite("virtual_owners",              &corgi::Communication::virtual_owners                      )
      

    //--------------------------------------------------
    // 1D
      
    py::module m_1d = m_base.def_submodule("oneD", "1D specializations");

    auto n1 = declare_node<1>(m_1d, "Grid");

    n1.def(py::init<size_t>());

    // make constructor symmetric and assume 3D shape
    n1.def(py::init( [](size_t nx, size_t /*ny*/) {
      //assert(ny == 1);

      return new corgi::Grid<1>(nx);}));
    n1.def(py::init( [](size_t nx, size_t /*ny*/, size_t /*nz*/) {
      // assert(ny == 1);
      // assert(nz == 1);
      return new corgi::Grid<1>(nx);}));

    // tile adding; this only works for D=1,2; otherwise py GC kills tiles
    n1.def("add_tile", &corgi::Grid<1>::add_tile, py::keep_alive<1,2>());

    n1.def("get_tile", [](corgi::Grid<1> &n, size_t i, size_t /*j*/){
        return n.get_tileptr_ind(i); },
        py::return_value_policy::reference,
        py::keep_alive<1,0>()
        );
    n1.def("get_tile", [](corgi::Grid<1> &n, size_t i, size_t /*j*/, size_t /*k*/){
        return n.get_tileptr_ind(i); },
        py::return_value_policy::reference,
        py::keep_alive<1,0>()
        );

    n1.def("get_Nx",   [](corgi::Grid<1> &n){ return n.get_Nx(); })
      .def("get_xmin", [](corgi::Grid<1> &n){ return n.get_xmin(); })
      .def("get_xmax", [](corgi::Grid<1> &n){ return n.get_xmax(); });

    n1.def("get_Ny",   [](corgi::Grid<1> &){ return 1; })
      .def("get_Nz",   [](corgi::Grid<1> &){ return 1; });

    n1.def("get_ymin", [](corgi::Grid<1> & ){ return 0.0; })
      .def("get_ymax", [](corgi::Grid<1> & ){ return 1.0; });

    n1
      .def("set_grid_lims", [](corgi::Grid<1> &n, double xmin, double xmax)
          { n.set_grid_lims({{xmin}}, {{xmax}}); });

    n1
      .def("set_grid_lims", [](corgi::Grid<1> &n, double xmin, double xmax,
                                                double /*ymin*/, double /*ymax*/
            )
          { n.set_grid_lims({{xmin}}, {{xmax}}); });
    n1
      .def("set_grid_lims", [](corgi::Grid<1> &n, double xmin, double xmax,
                                                double /*ymin*/, double /*ymax*/,
                                                double /*zmin*/, double /*zmax*/
            )
          { n.set_grid_lims({{xmin}}, {{xmax}}); });


    // mpi grid py bindings
    n1
      .def("get_mpi_grid", [](corgi::Grid<1> &n, const size_t i){ 
          const auto val = n.py_get_mpi_grid(i); 
          return val;
          })
      .def("get_mpi_grid", [](corgi::Grid<1> &n, const size_t i, const size_t ){ 
          const auto val = n.py_get_mpi_grid(i); 
          return val;
          })
      .def("set_mpi_grid", [](corgi::Grid<1> &n, size_t i, int val){ n.py_set_mpi_grid(val, i); })
      .def("set_mpi_grid", [](corgi::Grid<1> &n, size_t i, size_t /*j*/, int val){ n.py_set_mpi_grid(val, i); });

    // work grid py bindings
    n1
      .def("get_work_grid", [](corgi::Grid<1> &n, const size_t i){ 
          const auto val = n.py_get_work_grid(i); 
          return val;
          })
      .def("get_work_grid", [](corgi::Grid<1> &n, const size_t i, const size_t ){ 
          const auto val = n.py_get_work_grid(i); 
          return val;
          })
      .def("set_work_grid", [](corgi::Grid<1> &n, size_t i, double val){ n.py_set_work_grid(val, i); })
      .def("set_work_grid", [](corgi::Grid<1> &n, size_t i, size_t /*j*/, double val){ n.py_set_work_grid(val, i); })

      .def("id", [](const corgi::Grid<1> &n, const size_t i){ return n.id(i);});
      

    //--------------------------------------------------
    auto t1 = declare_tile<1>(m_1d, "Tile");
    t1.def("neighs", [](corgi::Tile<1> &t, int i ){ return t.neighs(i); });



    //--------------------------------------------------
    // 2D
    py::module m_2d = m_base.def_submodule("twoD", "2D specializations");


    auto n2 = declare_node<2>(m_2d, "Grid");
    n2.def(py::init<size_t, size_t>());

    n2.def(py::init( [](size_t nx, size_t ny, size_t /*nz*/) {
      //assert(nz == 1);
      return new corgi::Grid<2>(nx, ny);}));

    // tile adding; this only works for D=1,2; otherwise py GC kills tiles
    n2.def("add_tile", &corgi::Grid<2>::add_tile, py::keep_alive<1,2>());

    n2.def("get_Nx",   [](corgi::Grid<2> &n){ return n.get_Nx(); })
      .def("get_Ny",   [](corgi::Grid<2> &n){ return n.get_Ny(); });

    n2.def("get_Nz",   [](corgi::Grid<2> &){  return 1; });

    n2.def("get_xmin", [](corgi::Grid<2> &n){ return n.get_xmin(); })
      .def("get_xmax", [](corgi::Grid<2> &n){ return n.get_xmax(); })
      .def("get_ymin", [](corgi::Grid<2> &n){ return n.get_ymin(); })
      .def("get_ymax", [](corgi::Grid<2> &n){ return n.get_ymax(); });


    n2.def("get_tile", [](corgi::Grid<2> &n, size_t i, size_t j){ 
          return n.get_tileptr( std::make_tuple(i,j) ); },
            py::return_value_policy::reference,
            py::keep_alive<1,0>()
          )
      .def("get_tile", [](corgi::Grid<2> &n, size_t i, size_t j, size_t /*k*/){
        return n.get_tileptr_ind(i,j); },
            py::return_value_policy::reference,
            py::keep_alive<1,0>()
        )

      .def("set_grid_lims", [](corgi::Grid<2> &n, 
            double xmin, double xmax, 
            double ymin, double ymax)
          { n.set_grid_lims({{xmin,ymin}}, {{xmax, ymax}}); })
      .def("set_grid_lims", [](corgi::Grid<2> &n, 
            double xmin, double xmax, 
            double ymin, double ymax,
            double /*zmin*/, double /*zmax*/)
          { n.set_grid_lims({{xmin,ymin}}, {{xmax, ymax}}); })

      .def("get_mpi_grid", [](corgi::Grid<2> &n, const size_t i, const size_t j){ 
          const auto val = n.py_get_mpi_grid(i,j); 
          return val;
          })
      .def("set_mpi_grid", [](corgi::Grid<2> &n, size_t i, size_t j, int val){ n.py_set_mpi_grid(val, i, j); })

      .def("get_work_grid", [](corgi::Grid<2> &n, const size_t i, const size_t j){ 
          const auto val = n.py_get_work_grid(i,j); 
          return val;
          })
      .def("set_work_grid", [](corgi::Grid<2> &n, size_t i, size_t j, double val){ n.py_set_work_grid(val, i, j); })

      .def("id", [](const corgi::Grid<2> &n, const size_t i, const size_t j){ return n.id(i,j);});


    auto t2 = declare_tile<2>(m_2d, "Tile");
    t2.def("neighs", [](corgi::Tile<2> &t, int i, int j){ return t.neighs(i,j); });


    //--------------------------------------------------
    // 3D
    py::module m_3d = m_base.def_submodule("threeD", "3D specializations");

    auto n3 = declare_node<3>(m_3d, "Grid");
    n3.def(py::init<size_t, size_t, size_t>());

    n3.def("get_Nx",   [](corgi::Grid<3> &n){ return n.get_Nx(); })
      .def("get_Ny",   [](corgi::Grid<3> &n){ return n.get_Ny(); })
      .def("get_Nz",   [](corgi::Grid<3> &n){ return n.get_Nz(); });

    n3.def("get_xmin", [](corgi::Grid<3> &n){ return n.get_xmin(); })
      .def("get_xmax", [](corgi::Grid<3> &n){ return n.get_xmax(); })
      .def("get_ymin", [](corgi::Grid<3> &n){ return n.get_ymin(); })
      .def("get_ymax", [](corgi::Grid<3> &n){ return n.get_ymax(); })
      .def("get_zmin", [](corgi::Grid<3> &n){ return n.get_zmin(); })
      .def("get_zmax", [](corgi::Grid<3> &n){ return n.get_zmax(); });

    n3.def("add_tile", &corgi::Grid<3>::add_tile, py::keep_alive<1,2>());

    n3.def("get_tile", [](corgi::Grid<3> &n, size_t i, size_t j, size_t k)
        { 
          return n.get_tileptr( std::make_tuple(i,j,k) ); 
        },
        py::return_value_policy::reference,
        // keep alive for the lifetime of the grid
        //
        // pybind11:
        // argument indices start at one, while zero refers to the return 
        // value. For methods, index one refers to the implicit this pointer, 
        // while regular arguments begin at index two. 
        // py::keep_alive<nurse,patient>()
        py::keep_alive<1,0>()
        )

      .def("set_grid_lims", [](corgi::Grid<3> &n, 
            double xmin, double xmax, 
            double ymin, double ymax,
            double zmin, double zmax)
          { n.set_grid_lims({{xmin,ymin,zmin}}, {{xmax, ymax,zmax}}); })

      .def("get_mpi_grid", [](corgi::Grid<3> &n, const size_t i, const size_t j, const size_t k){ 
          const auto val = n.py_get_mpi_grid(i,j,k); 
          return val;
          })
      .def("set_mpi_grid", [](corgi::Grid<3> &n, size_t i, size_t j, size_t k, int val){ 
          n.py_set_mpi_grid(val, i, j, k); })

      .def("get_work_grid", [](corgi::Grid<3> &n, const size_t i, const size_t j, const size_t k){ 
          const auto val = n.py_get_work_grid(i,j,k); 
          return val;
          })
      .def("set_work_grid", [](corgi::Grid<3> &n, size_t i, size_t j, size_t k, double val){ 
          n.py_set_work_grid(val, i, j, k); })

      .def("id", [](const corgi::Grid<3> &n, const size_t i, const size_t j, const size_t k){ 
          return n.id(i,j,k);});

    auto t3 = declare_tile<3>(m_3d, "Tile");
    t3.def("neighs", [](corgi::Tile<3> &t, int i, int j, int k){ return t.neighs(i,j,k); });


}




