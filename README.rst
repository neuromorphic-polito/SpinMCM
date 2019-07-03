SpinMCM: Multicast Messaging for SpiNNaker
------------------------------------------

SpinMCM is a library providing an interface for performing broadcast transmissions among the cores of the SpiNNaker neuromorphic platform. It also implements a synchronisation mechanism among the cores. Both Broadcast and Sync functionalities are implemented by exploiting the multicast capabilities of the board.

Installation
------------

1. Download SpinMCM to your project folder. 

2. In the SpinMCM folder, build and install the C libraries:

::

	% make install

Usage instructions
------------------

To use SpinMCM in your SpiNNaker application, include the header files in your C file:

::

	#include "PATH_TO/spin2_api.h"

Authorship and copyright
------------------------

SpinMCM is being developed by `Francesco Barchi <mailto:francesco.barchi@polito.it>`__.

+------------------------------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| **Please respect the license of the software**                                                                                                                                                                                        |
+------------------------------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+
| .. image:: https://user-images.githubusercontent.com/7613428/60581999-4168a180-9d88-11e9-87e3-ce5e127b84a1.png   | SpinMCM is free and open source software, so you can use it for any purpose, free of charge.                       |
|    :alt: Respect the license                                                                                     | However, certain conditions apply when you (re-)distribute and/or modify SpinMCM; please respect the               |
|    :target: https://github.com/neuromorphic-polito/SpinMCM/blob/master/LICENSE.rst                               | `license <https://github.com/neuromorphic-polito/SpinMCM/blob/master/LICENSE.rst>`__.                            |
|    :width: 76px                                                                                                  |                                                                                                                    |
+------------------------------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------+

*icons on this page by Austin Andrews / https://github.com/Templarian/WindowsIcons*
