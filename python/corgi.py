import os, sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm

import palettable as pal
cmap = pal.wesanderson.Moonrise1_5.mpl_colormap

import corgi


Nrank = 4



##################################################
# plotting tools

# visualize matrix
def imshow(ax, grid):
    ax.clear()
    ax.minorticks_on()
    ax.set_xlim(corgi.xmin, corgi.xmax)
    ax.set_ylim(corgi.xmin, corgi.xmax)

    extent = [corgi.xmin, corgi.xmax, corgi.ymin, corgi.ymax]

    mgrid = np.ma.masked_where(grid == -1.0, grid)
    ax.imshow(mgrid,
              extent=extent,
              origin='lower',
              interpolation='nearest',
              cmap = cmap,
              vmin = 0.0,
              vmax = Nrank-1,
              aspect='auto',
              #vmax = Nrank,
              #alpha=0.5
              )


# Visualize current cell ownership on node
def plot_node(ax, n, lap):
    tmp_grid = np.ones( (corgi.Nx, corgi.Ny) ) * -1.0

    
    #for i in range(corgi.Nx):
    #    for j in range(corgi.Ny):
    #        cid = n.cell_id(i,j)
    #        if n.is_local(cid):
    #            tmp_grid[i,j] = 0.5


    for cid in n.get_cells():
        c = n.get_cell( cid )
        (i, j) = c.index()
        #check dublicates
        if tmp_grid[i,j] != -1.0:
            print "{}: ERROR in real cells at ({},{})".format(n.rank, i,j)
            sys.exit()
        tmp_grid[i,j] = c.owner


    for cid in n.get_virtuals():
        c = n.get_cell( cid )
        (i,j) = c.index()
        if tmp_grid[i,j] != -1.0:
            print "{}: ERROR in virtual cells at ({},{})".format(n.rank, i,j)
            sys.exit()
        tmp_grid[i,j] = c.owner

    imshow(ax, tmp_grid)
    #imshow(ax, n.mpiGrid)


    # add text label about number of neighbors
    for cid in n.get_cells():
        c = n.get_cell( cid )
        (i, j) = c.index()
        ix = corgi.xmin + corgi.xmax*(i+0.5)/corgi.Nx
        jy = corgi.ymin + corgi.ymax*(j+0.5)/corgi.Ny

        #Nv = n.number_of_virtual_neighbors(c)
        Nv = c.number_of_virtual_neighbors
        label = str(Nv)
        #label = "{} ({},{})/{}".format(cid,i,j,Nv)
        #label = "({},{})".format(i,j)
        ax.text(jy, ix, label, ha='center',va='center', size=8)


    #for cid in n.get_virtuals():
    #    c = n.get_cell( cid )
    #    (i,j) = c.index()
    #    ix = corgi.xmin + corgi.xmax*(i+0.5)/corgi.Nx
    #    jy = corgi.ymin + corgi.ymax*(j+0.5)/corgi.Ny
    #    label = "Vir"
    #    ax.text(jy, ix, label, ha='center',va='center')


    ax.set_title(str(len(n.get_virtuals() ))+"/"+str(len(n.get_cells() )))


    #save
    slap = str(lap).rjust(4, '0')
    fname = fpath + '/node_{}_{}.png'.format(n.rank, slap)
    plt.savefig(fname)






if __name__ == "__main__":


    ################################################## 
    # set up plotting and figure
    plt.fig = plt.figure(1, figsize=(8,4))
    plt.rc('font', family='serif', size=12)
    plt.rc('xtick')
    plt.rc('ytick')
    
    gs = plt.GridSpec(1, 2)
    gs.update(hspace = 0.5)
    
    axs = []
    axs.append( plt.subplot(gs[0]) )
    axs.append( plt.subplot(gs[1]) )


    c = corgi.Cell( 0, 0, 0 )
    print "cid:", c.index()
    print "owner", c.owner
    print "neigs", c.neighs(-1, -1)
    print "nhood:", c.nhood()

    print "--------------------------------------------------"
    print "testing node..."

    n = corgi.Node()
    n.init_mpi()

    # Path to be created (avoid clutter by issuing this with master)
    fpath = "out/"
    if n.master:
        if not os.path.exists(fpath):
            os.makedirs(fpath)



    #make random starting order
    if False:
        np.random.seed(4)
        if n.master:
            for i in range(corgi.Nx):
                for j in range(corgi.Ny):
                    val = np.random.randint(n.Nrank)
                    n.set_mpiGrid(i, j, val)

    # Normal side-by-side stride start
    if True:
        if n.master:
            stride = np.zeros( (corgi.Ny), np.int64)
            dy = np.float(corgi.Ny) / np.float(n.Nrank) 
            for j in range(corgi.Ny):
                val = np.int( j/dy )
                stride[j] = val

            for i in range(corgi.Nx):
                for j in range(corgi.Ny):
                    val = stride[j]
                    n.set_mpiGrid(i, j, val)

    n.bcast_mpiGrid()

    #for i in range(corgi.Nx):
    #    for j in range(corgi.Ny):
    #        print "{}: ({},{}) = {}".format(n.rank, i,j,n.mpiGrid(i,j))
    #sys.exit()

    #for i in range(corgi.Nx):
    #    for j in range(corgi.Ny):
    #        val = np.random.randint(n.Nrank)
    #        n.set_mpiGrid(i, j, val)



    #load cells into local node
    for i in range(corgi.Nx):
        for j in range(corgi.Ny):
            #print "{} ({},{}) {} ?= {}".format(n.rank, i,j, n.mpiGrid(i,j), ref[j,i])
            if n.mpiGrid(i,j) == n.rank:
                c = corgi.Cell(i, j, n.rank)

                #TODO load data to cell
                n.add_local_cell(c)



    plot_node(axs[0], n, 0)

    n.analyze_boundary_cells()
    print n.rank, ": send queue        : ", n.send_queue
    print n.rank, ": send queue address: ", n.send_queue_address

    n.communicate_send_cells()
    n.communicate_recv_cells()
    plot_node(axs[0], n, 1)



    n.finalize_mpi()
