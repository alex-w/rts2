<?php
	session_start ();

	include "config.php";

	function hlavicka ($title, $headline, $ok, $status)
	{
		include "menu.php";
		head (NAME . ' - ' . $title);
	}

	function konec ()
	{
		echo <<<EOT
	</td></tr></tb>
	</table>
	</body>
	</html>
EOT;
	}

	function print_night ($n) {
		$t = $n * 86400;
		echo "<a href='night.php?night=$n'><b>Night</b></a> from <b>" . date ("d.m.Y", $t) . " 12:00 UT</b> to <b>" . date ("d.m.Y", $t + 86400) . " 12:00 UT</b>.<br>\n";
	}

	function s2deg ($s, $mul) {
		$i = 0;
		$len = strlen ($s);
		$res = '';
		$sign = 1;
		for ($i = 0; $i < $len && (($c = $s[$i]) <= '9' && $c >= '0') || $c == '+' || $c == '-' || $c == '.'; $i++)
		{
			if ($c == '-')
				$sign = -1;
			$res .= $c;
		}
		if ($i == $len)
			return floatval($res);
		$res = abs ($res);
		$out = $mul * $res;
		$mul = $mul / 60.0;
		$res = '';
		for ($i++; $i < $len; $i++) {
			$c = $s[$i];
			if ($res > '' && ($c > '9' || $c < '0') && $c != '.') {
				$out += floatval($res) * $mul;
				$mul /= 60.0;
				$res = '';
			}
			elseif ($c == '.' || ($c > '0' && $c < '9'))
			{
				$res .= $c;
			}
		}
		return $sign * ($out + floatval($res) * $mul);
	}

	function deg2s ($d) {
		$sign = $d <= 0 ? '-' : '+';
		$d = abs ($d);
		$h = floor ($d);
		$ret = sprintf ("$sign%d&deg;", $h);
		$d = round (fmod ($d, 1) * 60.0, 3);
		$ret = $ret . sprintf ("%02.0f'", floor($d));
		$s = fmod ($d, 1) * 60.0;
		return $ret . sprintf ("%02.1f''", $s);
	}

	function hour2s ($d) {
		$a = $d;
		$d = $d / 15.0;
		$h = floor ($d);
		$ret = sprintf ("%02d:", $h);
		$d = round (fmod ($d, 1) * 60.0, 3);
		$ret = $ret . sprintf ("%02.0f:", floor($d));
		$s = fmod ($d, 1) * 60.0; 
		return $ret . sprintf ("%02.1f", $s);
	}

	function get_value ($fname, $value) {
		if ($fname == 'tar_ra')
			return hour2s ($value) . "($value)";
		else if ($fname == 'tar_dec')
			return deg2s ($value) . "($value)";
		else if ($fname == 'tar_enabled')
			return $value == 't' ? 'enabled' : 'disabled';
		else return $value;
	}		

	$fields_name = array (
		'camera_name' => 'Camera name',
		'grb_id' => "GRB #",
		'grb_seqn' => "GRB Seq.",
		'grb_date' => "GRB Date",
		'grb_last_update' => "GRB last update",
		'img_path' => "Image preview",
		'img_count' => 'IC',
		'img_date' => "Image date",
		'img_exposure_sec' => "E ('')",
		'img_temperature_deg' => "T (&deg;C)",
		'img_filter' =>	"F",
		'tar_id' => 'Tar. ID',
		'tar_name' => 'Target name',
		'tar_comment' => 'Target comment',
		'tar_ra' => 'Target RA',
		'tar_dec' => 'Target DEC',
		'tar_enabled' => 'Target enabled',
		'obs_id' => 'Obs. ID',
		'obs_start' => 'Observation start',
		'obs_duration' => 'Duration',
		'ot_imgcount' => 'Images per night',
		'ot_minpause' => 'Minimal pause',
		'ot_priority' => 'Priority',
		'type_id' => 'Type id',
		'type_description' => 'Description',
		'med_id' => 'Media ID',
		'med_path' => 'Media path'
	);
		
	$fields_links = array (
		'tar_id' => "targets.php?tar_id=",
		'obs_id' => "observations.php?obs_id=",
		'type_id' => "types.php?type_id=",
		'med_id' => "medias.php?med_id="
	);

	$fields_writable = array (
		'grb_id' => 1,
		'grb_seqn' => 1,
		'grb_date' => 1,
		'ot_imgcount' => 1,
		'ot_minpause' => 1,
		'ot_priority' => 1,
		'type_id' => 1, 
		'type_description' => 1, 
		'tar_name' => 1 , 
		'tar_ra' => 1, 
		'tar_dec' => 1, 
		'tar_type' => 1, 
		'tar_comment' => 1,
		'tar_enabled' => 1,
		'med_path' => 1
	);

	$fields_types = array (
		'tar_enabled' => 'b'
	);
	
	class Query {
		var $fields;
		var $from;
		var $where;
		var $order;

		var $con;
		var $res;

		function Query () {
			$this->clear ();
		}

		function clear () {
			$this->fields = "";
			$this->from = "";
			$this->where = "";
			$this->order = "";
		}

		function error ($message) {
			echo "<div class='error'>$message</div>\n";
		}

		function simple_query ($query_string) {
			if (!$this->con)
				$this->connect ();
			$res = pg_query ($this->con, $query_string);
			if (pg_num_rows($res))
			{
				return  pg_fetch_result ($res, 0, 0);
			}
			else
			{
				return 0;
			}
		}

		function add_field ($f) {
			$this->fields .= ($this->fields ? "," : "") . $f;
		}

		function add_from ($t) {
			$this->from .= ($this->from ? "," : "") . $t;
		}
		
		function add_and_where ($w) {
			$this->where .= ($this->where ? " AND " : " WHERE ") . $w;
		}

		function add_and_test_where ($t, $w) {
			if (array_key_exists($w, $_SESSION))
				$this->add_and_where ("$t '$_SESSION[$w]'");
		}

		function add_order ($o) {
			$this->order .= ($this->order ? "," : " ORDER BY ") . $o;
		}

		function build () {
			return "SELECT " . $this->fields . " FROM " .
					$this->from . $this->where . $this->order . ";";
			}

		function connect ($dbname = "dbname=stars") {
			$this->con = pg_connect ($dbname);
		}

		function do_query ($sql = "") {
			if (!$this->con)
				$this->connect ();
			if ($sql == "")
				$sql = $this->build ();
			$this->res = pg_query ($this->con, $sql);
			if ($this->res == false)
			{
				$this->error ("PgSQL error: " . pg_last_error ($this->con) . " [ when executing '$sql'");
			}
			return $this->res;
		}

		function print_pages ($page, $pgs, $r, $limit, $count) {
			$from = max (1, $page - 10);
			$to_count = intval ($count/$pgs) + 1;
			$to = min ($to_count, $page + 9);
			if ($from == 1 && $to == 1) {
				echo "All $limit records are shown.<br>";
			} else {
				$url = $_SERVER['REQUEST_URI'];
				$url = preg_replace ('/&?page=\d+/', '', $url);
				if (strstr ($url, '?'))
					$url .= '&';
				else
					$url .= '?';
					
				if ($from != 1) 
					echo "<a href='$url" . "page=" . max (1, $from - 10) . "'>&lt;&lt;</a>&nbsp;";
				for ($i = $from; $i <= $to; $i++)
					if ($i != $page)
						echo "<a href='$url" . "page=$i'>$i</a>&nbsp;";
					else
						echo "$i&nbsp;";
				if ($to != $to_count)
					echo "<a href='$url" . "page=" . min ($to_count, $to + 10) . "'>&gt;&gt;</a>&nbsp;";
				echo "&nbsp;&nbsp;Records from " . ($r+1) ." to $limit out of $count.<br>\n";
			}
		}

		function print_table () {
			global $fields_name;
			global $fields_links;
			$count = pg_numrows ($this->res);
			if ($count == 0) {
				echo "Cannot find any row matching query";
				return -1;
			}
			$r = 0;
			$pgs = $count;
			if (!array_key_exists ('page_size', $_SESSION))
				$_SESSION['page_size'] = 100;
			$pgs = $_SESSION['page_size'];
			if (array_key_exists ('page', $_REQUEST))
				$page = intval ($_REQUEST['page']);
			else
				$page = 1;
			if ($page == 0)
				$page = 1;
			else
				$r = ($page - 1) * $pgs;
			$limit = min($r + $pgs, $count);
			$this->print_pages ($page, $pgs, $r, $limit, $count);
			$img_form = 0;
			if ($_SESSION['authorized']) {
				for ($i = 0; $i < pg_numfields($this->res); $i++) {
					if (pg_fieldname($this->res, $i) == 'img_path')
					{
						echo "<form action='download.php' method='post'>\n";
						$img_form = 1;
					}
				}
			}
			echo "<table cellspacing='0' cellpadding='0'>\n";
			echo "<th><tr>\n";
			for ($i = 0; $i < pg_numfields($this->res); $i++) {
				$name = pg_fieldname($this->res, $i);
				echo "\t\t<td class='" . (($i % 2) ? "even" : "head") . "'>" . (array_key_exists ($name, $fields_name) ? $fields_name[$name] : $name) . "</td>\n";
			}
			echo "</tr></th>\n<tb>\n";
			while ($r < $limit && ($row = pg_fetch_row ($this->res, $r))) {
				echo "<tr>\n";
				for ($i = 0; $i < pg_numfields($this->res); $i++) {
					$fname = pg_fieldname($this->res, $i);
					echo "<td class='" . (($i % 2) ? 'odd' : "even") . "'>";
					if ($row[$i] == "")
						echo "und";
					else if ($fname == 'img_path') {
						if ($_SESSION['authorized']) {
							echo "<input type='checkbox' name='$row[$i]' checked></input>";
						}
						echo "<a target='_new' href='preview.php?fn=$row[$i]&full=true'><img src='preview.php?fn=$row[$i]'></a>";
					}
					else if (array_key_exists($fname, $fields_links)) {
						echo "<a href='$fields_links[$fname]$row[$i]'>$row[$i]</a>";
					} else {
						echo get_value ($fname, $row[$i]);
					}
					echo "</td>\n";
				}
				echo "</tr>\n";
				$r++;
			}
			echo "</tb>\n</table>\n";
			if ($_SESSION['authorized'] && $img_form)
				echo "<input type='submit' value='Download selected'></inpu>\n</form>\n";
		}

		function print_radio_list ($table, $name, $desc, $selected) {
			if (!$this->con)
				$this->connect ();
			$res = pg_query ($this->con, "SELECT $name, $desc FROM $table ORDER BY $name");
			while ($row = pg_fetch_row ($res)) {
				echo "<input type='radio' name='$name' value='" . $row[0] . "'" . ($selected == $row[0] ? " checked" : "") . ">" . $row[1] . "</input><br/>\n";
			}
		}

		function print_field ($name, $value) {
			global $fields_name;
			global $fields_writable;
			global $fields_types;
			echo "\t<tr>\n\t\t<td>" . (array_key_exists ($name, $fields_name) ? $fields_name[$name] : $name) . 
			"\t\t</td><td>\n";
			if ($_SESSION['authorized']) {
				if ($name == "type_id") {
					echo "<input type='hidden' name='type_id_old' value='$value'></input>"; 
					$this->print_radio_list ("types", "type_id", "type_description", $value); 
				} elseif (array_key_exists($name, $fields_writable)) {
					switch ($fields_types[$name]) {
						case 'b':
							echo "<input type='radio' name='$name' value='t' " . 
								($value === 'enabled' ? 'checked' : '') . 
								">enabled</input>\n";
							echo "<input type='radio' name='$name' value='f' " . 
								($value === 'disabled' ? 'checked' : '') . 
								">disabled</input>\n";
							break;
						default:
							echo "<input type='text' name='$name' value=\"$value\"></input>";
							break;
					}
				} else {
					echo "<input type='hidden' name='$name' value=\"$value\">$value";
				}
			} else {
				echo $value;
			}
			echo "\n\t</td>\n\t</tr>\n";
		}

		function print_form () {
			global $fields_name;
			global $fields_writable;
			while ($row = pg_fetch_row ($this->res)) {
				if ($_SESSION['authorized'])
					echo "<form name='test' action='$_SERVER[REQUEST_URI]'>\n";
				echo "<table>\n";
				$ra = -500;
				$dec = -500;
				for ($i = 0; $i < pg_numfields($this->res); $i++) {
					$name = pg_fieldname($this->res, $i);
					if ($name == 'tar_ra')
						$ra = $row[$i];
					if ($name == 'tar_dec')
						$dec = $row[$i];
					$value = get_value ($name, $row[$i]);
					$this->print_field ($name, $value);
				}
				if ($ra != -500 && $dec != -500)
				{
					$q = new Query();
					echo "\t<tr>\n\t\t<td>All images with $ra, $dec</td>\n\t\t<td><a href='images.php?ra=$ra&dec=$dec&tar_id=&obs_id='>" .
					$q->simple_query("SELECT COUNT(*) FROM images WHERE isinwcs ($ra, $dec, astrometry);").
					"</a></td>\n\t</tr>\n";
					echo "</table>\n\t";
				}
				if ($_SESSION['authorized']) 
					echo "<input type='Submit' value='OK'></input>\n</form>\n";
			}
		}

		function print_form_insert () {
			global $fields_name;
			global $fields_writable;
			if ($_SESSION['authorized'])
			{
				echo "<form name='test' action='$_SERVER[REQUEST_URI]'>\n<table>";
				for ($i = 0; $i < pg_numfields($this->res); $i++) {
					$name = pg_fieldname($this->res, $i);
					echo "\t<tr>\n\t\t<td>" . (array_key_exists ($name, $fields_name) ? $fields_name[$name] : $name) . 
					"\t\t</td><td>\n<input type='text' name='$name'></input>\n\t</td>\n\t</tr>\n";
				}
				echo "</table>\n\t<input type='hidden' name='insert' value='1'></input>\n\t";
				echo "<input type='Submit' value='OK'></input>\n</form>\n";
			}
		}

		function print_list ($res) {
			echo "<ul>\n";
			while ($row = pg_fetch_row ($this->res))
				echo "\t<li>" . join ("&nbsp;", $row) . "</li>\n";
			echo "</ul<\n";
		}

		function close () {
			pg_close ();
		}
	}

	$settinngs_text = 
	array ('ra' => "Ra:", 'dec' => "Dec:", 'tar_id' => "Target id:",
		'obs_id' => "Observation id:", 'date_from' => 'Date from:',
		'date_to' => 'Date to:', 'page_size' => 'Page size:', 'camera_name' => 'Camera:',
		'mount_name' => 'Mount');

	$settinngs_size = 
	array ('ra' => 9, 'dec' => 11, 'tar_id' => 5,
		'obs_id' => 5, 'date_from' => 14,
		'date_to' => 14, 'page_size' => 3);

	function display_settings ($name, $lf = 0) {
		global $settinngs_text;
		global $settinngs_size;
		$text = array_key_exists ($name, $settinngs_text) ? $settinngs_text[$name] : "Unknow field: $name";
		$size = array_key_exists ($name, $settinngs_size) ? " size='$settinngs_size[$name]'" : "";
		echo <<<EOT
		<td valign="right">
			$text
		</td><td>
			<input type="text" name="$name" value="
EOT;
	if (array_key_exists($name, $_SESSION)) {
		if ($name == 'ra') {
			echo hour2s($_SESSION[$name]);
		} else if ($name == 'dec') {
			echo deg2s($_SESSION[$name]);
		} else {
			echo $_SESSION[$name];
		}
	} else {
		echo "";
	}
	echo <<<EOT
"$size></input>
	</td>
EOT;
	if ($lf)
		echo "</tr><tr>\n";
	}

	function get_str_val ($name) {
		if (array_key_exists($name, $_REQUEST)) {
			$_SESSION[$name] = $_REQUEST[$name];
			preg_replace ('/[^A-Za-z0-0 _.-]/', '', $_SESSION[$name]);
			if ($_SESSION[$name] == '')
				unset ($_SESSION[$name]);
		}
	}

	function get_deg_val ($name, $mul) {
		if (array_key_exists($name, $_REQUEST)) {
			if (is_numeric($_REQUEST[$name]))
				$_SESSION[$name] = floatval($_REQUEST[$name]);
			else if (preg_match('/^[+-]?\d+((.\d*)(\.\d+)?)*$/', $_REQUEST[$name]))
				$_SESSION[$name] = s2deg ($_REQUEST[$name], $mul);
			else
				unset ($_SESSION[$name]);
		}
	}

	function get_int_val ($name) {
		if (array_key_exists($name, $_REQUEST)) {
			if (is_numeric($_REQUEST[$name]))
				$_SESSION[$name] = intval($_REQUEST[$name]);
			else
				unset ($_SESSION[$name]);
		}
	}

	function get_date_val ($name) {
		if (array_key_exists($name, $_REQUEST)) {
			if ($_REQUEST && preg_match ("/^\d+-\d+-\d+$/", $_REQUEST[$name]))
				$_SESSION[$name]=$_REQUEST[$name];
			else
				unset ($_SESSION[$name]);
		}
	}

	get_deg_val ('ra', 15);
	get_deg_val ('dec', 1);
	get_int_val ('tar_id');
	get_int_val ('obs_id');
	get_date_val ('date_from');
	get_date_val ('date_to');
	get_int_val ('page_size');
	get_str_val ('camera_name');
	get_str_val ('mount_name');

	if (!array_key_exists('authorized', $_SESSION)) 
		$_SESSION['authorized'] = 0;

	//	$_SESSION['login'] = 'anonymous';

	if (array_key_exists('login', $_REQUEST) && array_key_exists('password', $_REQUEST))
	{
		$q = new Query();
		if ($q->simple_query ("SELECT COUNT(*) FROM users WHERE usr_login = '$_REQUEST[login]' and usr_passwd = '$_REQUEST[password]';"))
		{
			$_SESSION['authorized'] = 1;
			$_SESSION['login'] = $_REQUEST['login'];
		}
		$q->close ();
	}
?>
