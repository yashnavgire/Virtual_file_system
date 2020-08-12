# Virtual_file_system

-Project is about imitating and customizing the filesystem structure and providing the functionalities of normal system calls.

-The FileSystem is created and maintained on RAM when the application is executed.After exiting/closing the application files which are created on
 created filesystem are saved in new folder named 'savedvfs' in the working directory.

-Next time when application is exectued again ,all the files from './savedvfs' are retrieved in the virtual filesystem.

-Functionalities provided : create,read,write,truncate,remove,lseek,close etc.


--User can use --help command to know more after executing the application.
