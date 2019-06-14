from __future__ import print_function

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm
import palettable as pal
cmap = pal.wesanderson.Moonrise1_5.mpl_colormap

import sys, os

import pycorgi


Nrank = 4

#make random starting order
def loadMpiRandomly(n):
    np.random.seed(4)
    if n.master:
        for i in range(n.get_Nx()):
            for j in range(n.get_Ny()):
                val = np.random.randint(n.Nrank)
                n.set_mpi_grid(i, j, val)


#load nodes to be in stripe formation
def loadMpiStrides(n):
    if n.master: #only master initializes; then sends
        stride = np.zeros( (n.get_Ny()), np.int64)
        dy = np.float(n.get_Ny()) / np.float(n.Nrank) 
        for j in range(n.get_Ny()):
            val = np.int( j/dy )
            stride[j] = val

        for i in range(n.get_Nx()):
            for j in range(n.get_Ny()):
                val = stride[j]
                n.set_mpi_grid(i, j, val)
    n.bcast_mpi_grid()


#load cells into each grid
def loadCells(n):
    for i in range(n.get_Nx()):
        for j in range(n.get_Ny()):
            #print("{} ({},{}) {} ?= {}".format(n.rank, i,j, n.mpi_grid(i,j), ref[j,i]))
            if n.mpi_grid(i,j) == n.rank:
                c = pycorgi.Cell(i, j, n.rank, n.get_Nx, n.get_Ny)
                n.addLocalCell(c) #TODO load data to cell



##################################################
# plotting tools

# visualize matrix
def imshow(ax, grid, xmin, xmax, ymin, ymax):

    ax.clear()
    ax.minorticks_on()
    ax.set_xlim(xmin, xmax)
    ax.set_ylim(ymin, ymax)
    #ax.set_xlim(-3.0, 3.0)
    #ax.set_ylim(-3.0, 3.0)

    extent = [ xmin, xmax, ymin, ymax ]

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


# Visualize current cell ownership on grid
def plot_node(ax, n, lap):
    tmp_grid = np.ones( (n.get_Nx(), n.get_Ny()) ) * -1.0

    
    #for i in range(n.get_Nx()):
    #    for j in range(n.get_Ny()):
    #        cid = n.cell_id(i,j)
    #        if n.is_local(cid):
    #            tmp_grid[i,j] = 0.5


    for cid in n.getCells():
        c = n.getCell( cid )
        (i, j) = c.index()
        #check dublicates
        if tmp_grid[i,j] != -1.0:
            print("{}: ERROR in real cells at ({},{})".format(n.rank, i,j))
            sys.exit()
        tmp_grid[i,j] = c.owner


    for cid in n.get_virtuals():
        c = n.getCell( cid )
        (i,j) = c.index()
        if tmp_grid[i,j] != -1.0:
            print("{}: ERROR in virtual cells at ({},{})".format(n.rank, i,j))
            sys.exit()
        tmp_grid[i,j] = c.owner

    imshow(ax, tmp_grid, n.get_xmin(), n.get_xmax(), n.get_ymin(), n.get_ymax() )


    # add text label about number of neighbors
    for cid in n.getCells():
        c = n.getCell( cid )
        (j, i) = c.index()
        dx = n.get_xmax() - n.get_xmin()
        dy = n.get_ymax() - n.get_ymin()

        #NOTE annoying "feature" of swapping Nx and Ny because of imshow transpose
        ix = n.get_xmin() + dx*(i+0.5)/n.get_Ny()
        jy = n.get_ymin() + dy*(j+0.5)/n.get_Nx()

        #Nv = n.number_of_virtual_neighbors(c)
        Nv = c.number_of_virtual_neighbors
        label = str(Nv)
        #label = "{} ({},{})/{}".format(cid,i,j,Nv)
        #label = "({},{})".format(i,j)
        ax.text(ix, jy, label, ha='center',va='center', size=8)


    #for cid in n.get_virtuals():
    #    c = n.getCell( cid )
    #    (i,j) = c.index()
    #    ix = n.get_xmin() + n.get_xmax()*(i+0.5)/n.get_Nx()
    #    jy = n.get_ymin() + n.get_ymin()*(j+0.5)/n.get_Ny()
    #    label = "Vir"
    #    ax.text(jy, ix, label, ha='center',va='center')

    ax.set_title(str(len(n.get_virtuals() ))+"/"+str(len(n.getCells() )))

    #save
    slap = str(lap).rjust(4, '0')
    fname = fpath + '/node_{}_{}.png'.format(n.rank, slap)
    plt.savefig(fname)






if __name__ == "__main__":

    ################################################## 
    # setup environment
    xmin = -1.0
    xmax =  2.0
    ymin = -2.0
    ymax =  3.0

    pycorgi.setSize(10, 15)
    pycorgi.set_grid_lims(xmin, xmax, ymin, ymax)


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



    ################################################## 
    #init grid
    grid = pycorgi.Grid()
    grid.initMpi()

    loadMpiStrides(grid)
    loadCells(grid)

    # Path to be created 
    fpath = "out/"
    if grid.master:
        if not os.path.exists(fpath):
            os.makedirs(fpath)


    ################################################## 
    #visualize as a test
    plot_node(axs[0], grid, 0)


    ################################################## 
    # test step
    grid.analyzeBoundaryCells()
    print("{}: send queue        : {}".format(grid.rank, grid.send_queue))
    print("{}: send queue address: {}".format(grid.rank, grid.send_queue_address))

    grid.communicateSendCells()
    grid.communicateRecvCells()
    plot_node(axs[0], grid, 1)






    grid.finalizeMpi()


