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

# Projects using fpart or fpsync

* Harry Mangalam, from UCI, has an excellent article about data transfer
  [here](http://moo.nac.uci.edu/~hjm/HOWTO_move_data.html). Check out his
  [parsyncfp](https://github.com/hjmangalam/parsyncfp) and
  [parsyncfp2](https://github.com/hjmangalam/parsyncfp2) tools.

* Dave Altschuler wrote [dsync](https://github.com/daltschu11/dsync), a tool
  using fpart + rsync or rclone that can sync to the cloud

* [K-rsync](https://doughgle.github.io/k-rsync/) uses fpart and the kubernetes
  Job scheduler to transfer files between PVCs

* [fpsync-k8s-rclone](https://github.com/aboovv1976/fpsync-k8s-rclone) provides
  a patch for fpsync to make it support rclone and Kubernetes pods

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

* [Utah's CHPC](https://www.chpc.utah.edu/documentation/data_services.php#ptt)
  suggests fpart to transfer data in and out of CHPC resources

* Fpart is proudly referenced in the [French Government's 'SILL'](https://code.gouv.fr/sill/detail?name=fpart)

# Fpsync users (Storage / Cloud providers)

* [Intel](https://web.archive.org/web/20230314023512/http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/data-migration-enterprise-edition-for-lustre-software-white-paper.pdf)
  has written a white paper about data migration, presenting fpart and fpsync

* [Amazon](https://s3.amazonaws.com/solutions-reference/efs-backup/latest/efs-to-efs-backup.pdf)
  uses fpart and fpsync in their EFS-to-EFS backup solution. See also their
  [Amazon Elastic File System (Amazon EFS) for File Storage](https://www.slideshare.net/AmazonWebServices/amazon-elastic-file-system-amazon-efs-for-file-storage)
  presentation (AWS Storage Days, New York, September 6-8, 2017) and the
  [Amazon EFS performance tutorial](https://github.com/aws-samples/amazon-efs-tutorial/tree/master/performance),
  both presenting fpart and fpsync capabilities

* [Microsoft](https://learn.microsoft.com/en-us/azure/storage/files/storage-files-migration-nfs)
  suggests using fpart and fpsync to speed-up file transfers

* [Alibaba (Aliyun)](https://www.alibabacloud.com/help/doc-detail/128764.htm)

* [Oracle](https://docs.oracle.com/en-us/iaas/Content/File/Troubleshooting/transferring-windows-data-sms.htm)
  advises to use fpsync to copy data to File Storage. Check also their tutorial
  explaining how to use
  [migration Tools to Move Data into OCI Cloud Storage Services](https://docs.oracle.com/en/learn/migr-ocistorage-p1/)

* [cunoFS](https://cuno-cunofs.readthedocs-hosted.com/en/stable/user-guide-tips-for-apps.html#fpsync)

* [Nutanix](https://portal.nutanix.com/page/documents/solutions/details?targetId=TN-2016-Nutanix-Files-Migration-Guide:fpsync-for-nfs.html)

* [Huawei Latin America cloud](https://github.com/huaweicloud-latam/migration-tool-map/tree/main/02-014-fpsync)

<br>
<center>
[![Intel](img/ext-logos/Intel.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://web.archive.org/web/20230314023512/http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/data-migration-enterprise-edition-for-lustre-software-white-paper.pdf)
[![AWS](img/ext-logos/Aws.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://s3.amazonaws.com/solutions-reference/efs-backup/latest/efs-to-efs-backup.pdf)
[![Microsoft](img/ext-logos/Microsoft.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://learn.microsoft.com/en-us/azure/storage/files/storage-files-migration-nfs)
</center>
<br>
<center>
[![Alibaba](img/ext-logos/AlibabaCloud.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://www.alibabacloud.com/help/doc-detail/128764.htm)
[![Oracle](img/ext-logos/Oracle.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://docs.oracle.com/en-us/iaas/Content/File/Troubleshooting/transferring-windows-data-sms.htm)
[![cunoFS](img/ext-logos/cunoFS.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://cuno-cunofs.readthedocs-hosted.com/en/stable/user-guide-tips-for-apps.html#fpsync)
</center>
<br>
<center>
[![Nutanix](img/ext-logos/Nutanix.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://portal.nutanix.com/page/documents/solutions/details?targetId=TN-2016-Nutanix-Files-Migration-Guide:fpsync-for-nfs.html)
[![Huawei](img/ext-logos/Huawei.png){: style="width:175px;margin-right:75px;margin-down:75px"}](https://github.com/huaweicloud-latam/migration-tool-map/tree/main/02-014-fpsync)
</center>
