CREATE TABLE `mk_timeproc_log` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `task_id` int(10) NOT NULL DEFAULT '0',
  `result` tinyint(3) NOT NULL DEFAULT '1',
  `msg` text NOT NULL,
  `run_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 ;

CREATE TABLE `mk_timeproc` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `start_time` datetime NOT NULL,
  `end_time` datetime NOT NULL,
  `time_step` int(10) NOT NULL DEFAULT '1',
  `run_type` tinyint(3) NOT NULL DEFAULT '1',
  `proc_url` varchar(255) NOT NULL DEFAULT '',
  `memo` varchar(255) NOT NULL DEFAULT '',
  `timeout` int(10) NOT NULL DEFAULT '5',
  `last_run_time` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
