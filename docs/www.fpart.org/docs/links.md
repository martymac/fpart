# Resources

* See [fpart(1)](https://www.freebsd.org/cgi/man.cgi?query=fpart&apropos=0&sektion=0&manpath=FreeBSD+12.2-RELEASE+and+Ports&arch=default&format=html)
  and [fpsync(1)](https://www.freebsd.org/cgi/man.cgi?query=fpsync&apropos=0&sektion=0&manpath=FreeBSD+12.2-RELEASE+and+Ports&arch=default&format=html) for more details.

* Article about data migration using fpart and rsync (GNU Linux Magazine #164 - October 2013, french) :
  [Parallélisez vos transferts de fichiers](http://connect.ed-diamond.com/GNU-Linux-Magazine/GLMF-164/Parallelisez-vos-transferts-de-fichiers)

* [Steve French](https://lwn.net/Articles/789623/)
  mentioned fpart and fpsync at 2019 Linux Storage, Filesystem, and Memory-Management Summit (LSFMM)

* The [partition problem](http://en.wikipedia.org/wiki/Partition_problem) and
  [bin packing problem](http://en.wikipedia.org/wiki/Bin_packing_problem)
  descriptions on Wikipedia

* I am sure you will also be interested in [Packo](https://github.com/jbd/packo)
  which was developed by Jean-Baptiste Denis as the original proof of concept.
  See also his newer tool, [Msrsync](https://github.com/jbd/msrsync)

# Projects using fpart

* Harry Mangalam, from UCI, has an excellent article about data transfer
  [here](http://moo.nac.uci.edu/~hjm/HOWTO_move_data.html). Check out his
  [parsyncfp](https://github.com/hjmangalam/parsyncfp) and
  [parsyncfp2](https://github.com/hjmangalam/parsyncfp2) tools.

* Dave Altschuler wrote [dsync](https://github.com/daltschu11/dsync), a tool
  using fpart + rsync or rclone that can sync to the cloud

* [K-rsync](https://doughgle.github.io/k-rsync/) uses fpart and the kubernetes
  Job scheduler to transfer files between PVCs

# Fpsync users (Research / Education)

* [Bioteam](https://www.slideshare.net/chrisdag/practical-petabyte-pushing)
  used fpart + fpsync + rsync to migrate 2 PB of data

* [FAS RC (Harvard University)](https://www.rc.fas.harvard.edu/resources/documentation/transferring-data-on-the-cluster/#fpsync)
   writes about fpsync to move data on Harvard's Odyssey cluster

* [Standford University's](https://www.sherlock.stanford.edu/docs/software/list/)
  Sherlock HPC cluster offers fpart as a file management tool

* [Nantes University's](https://bird2cluster.univ-nantes.fr/news/rappel_transfert_02/)
  BiRD cluster provides a fpart module

* [Sweden's NSC](https://www.nsc.liu.se/support/storage/snic-centrestorage/moving-data/)
  (National Supercomputer Centre)'s Centre Storage suggests using fpsync

* [National Energy Research Scientific Computing Center](https://www.spectrumscaleug.org/wp-content/uploads/2019/10/HPCXXL19-NERSC-Site-update.pdf)
  (NERSC) writes about fpsync

* Fpart is proudly referenced in the [French Government's 'SILL'](https://sill.etalab.gouv.fr/software?name=fpart)

# Fpsync users (Cloud providers)

* [Intel](http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/data-migration-enterprise-edition-for-lustre-software-white-paper.pdf)
  has written a white paper about data migration, presenting fpart and fpsync

* [Amazon](https://aws.amazon.com/solutions/implementations/efs-to-efs-backup-solution/)
  uses fpart and fpsync in their EFS-to-EFS backup solution. See also their
  [Amazon Elastic File System (Amazon EFS) for File Storage](https://www.slideshare.net/AmazonWebServices/amazon-elastic-file-system-amazon-efs-for-file-storage) presentation
  (AWS Storage Days, New York, September 6-8, 2017) and the
  [Amazon EFS performance tutorial](https://github.com/aws-samples/amazon-efs-tutorial/tree/master/performance),
  both presenting fpart and fpsync capabilities

* [Microsoft](https://docs.microsoft.com/en-us/azure/storage/files/storage-troubleshoot-linux-file-connection-problems#slow-file-copying-to-and-from-azure-files-in-linux)
  suggests using fpart and fpsync to speed-up file transfers

* [Alibaba (Aliyun)](https://www.alibabacloud.com/help/doc-detail/128764.htm)
  has the same advice

* [Oracle](https://docs.oracle.com/en-us/iaas/Content/File/Troubleshooting/transferring-windows-data-sms.htm)
  too

<br>
<center>
[![Intel](img/ext-logos/Intel.png){: style="width:175px;margin-right:75px;margin-down:75px"}](http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/data-migration-enterprise-edition-for-lustre-software-white-paper.pdf)
[![AWS](img/ext-logos/Aws.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://aws.amazon.com/solutions/implementations/efs-to-efs-backup-solution/)
[![Microsoft](img/ext-logos/Microsoft.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://docs.microsoft.com/en-us/azure/storage/files/storage-troubleshoot-linux-file-connection-problems#slow-file-copying-to-and-from-azure-files-in-linux)
</center>
<br>
<center>
[![Alibaba](img/ext-logos/AlibabaCloud.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://www.alibabacloud.com/help/doc-detail/128764.htm)
[![Oracle](img/ext-logos/Oracle.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://docs.oracle.com/en-us/iaas/Content/File/Troubleshooting/transferring-windows-data-sms.htm)
</center>
