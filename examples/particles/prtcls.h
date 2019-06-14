#pragma once

#include <vector>
#include <mpi4cpp/mpi.h>

#include "../../tile.h"
#include "../../corgi.h"

#include "container.h"
#include "communication.h"


namespace prtcls {



/// Particle tile
class Tile : public corgi::Tile<2> {

  public:

  typedef Tile Tile_t;
  typedef std::shared_ptr<Tile> Tileptr;

  Tile() {};
  ~Tile() = default;

  /// particle storage
  std::vector<ParticleBlock> containers;

  /// getter
  ParticleBlock& get_container(size_t i) { return containers[i]; };

  /// setter
  void set_container(const ParticleBlock& block) {containers.push_back(block);};

  size_t Nspecies() {return containers.size(); };
    


  //--------------------------------------------------
  // MPI send
  virtual std::vector<mpi4cpp::mpi::request> 
  send_data( mpi4cpp::mpi::communicator&, int orig, int mode, int tag) override;

  /// actual tag=0 send
  std::vector<mpi4cpp::mpi::request> 
  send_particle_data( mpi4cpp::mpi::communicator&, int orig, int tag);

  /// actual tag=1 send
  std::vector<mpi4cpp::mpi::request> 
  send_particle_extra_data( mpi4cpp::mpi::communicator&, int orig, int tag);


  //--------------------------------------------------
  // MPI recv
  virtual std::vector<mpi4cpp::mpi::request> 
  recv_data(mpi4cpp::mpi::communicator&, int dest, int mode, int tag) override;

  /// actual tag=0 recv
  std::vector<mpi4cpp::mpi::request> 
  recv_particle_data(mpi4cpp::mpi::communicator&, int dest, int tag);

  /// actual tag=1 recv
  std::vector<mpi4cpp::mpi::request> 
  recv_particle_extra_data(mpi4cpp::mpi::communicator&, int dest, int tag);
  //--------------------------------------------------


  /// check all particle containers for particles
  // exceeding limits
  void check_outgoing_particles();

  /// delete particles from each container that are exceeding
  // the boundaries
  void delete_transferred_particles();

  /// get particles flowing into this tile
  void get_incoming_particles(corgi::Grid<2>& grid);

  /// pack particles for MPI message
  void pack_outgoing_particles();

  /// unpack received MPI message particles
  void unpack_incoming_particles();

  /// delete all particles from each container
  void delete_all_particles();

};



/// Particle mover
class Pusher {

  public:
    void solve(Tile&);
};



} // end of namespace prtcls
