#### Description
This application fetches torrent files of your favourite TV shows from your trackers of choice. Use cron for scheduling and torrent application with watchdir support.

#### History
tvfetch came to life after FlexGet running on RaspberryPi proved to be too slow to be useable.

#### Dependencies
* libcurl
* libxml2
* libpcre

#### Compilation
`make`

Compiled binary will appear in bin directory.

#### Configuration
Sample session.xml is provided in sample directory.
* \<target\> - where to download torrent files (torrent client's watchdir)
* \<feeds\>
  * \<feed\>
    * \<name\> - name of tracker, used only in log messages
    * \<url\> - URL to torrent tracker's RSS feed
    * \<delay\> - you can prioritize trackers by giving them delay (in seconds), larger delay - lower priority (set to 0 for instant download)
* \<shows\>
  * \<show\>
    * \<regex\> - PCRE of the show title to fetch
      * first group must match season number
      * second group must match episode number
      * no more groups are allowed
    * \<season\> - season number of the last downloaded episode (0 for new show)
    * \<episode\> - episode number of the last downloaded episode (0 for new season)
    * \<seen\> - timestamp of when the episode was first seen in any of the feeds, used by delay system (set to 0 for new entry)

#### Running
`./tvfetch /path/to/session.xml`

Sample cron job:  
`*/15 * * * * /path/to/tvfetch /path/to/session.xml >> /path/to/fetch.log 2>> /path/to/error.log`  
Runs every 15 minutes forwarding output to a dedicated log files.
