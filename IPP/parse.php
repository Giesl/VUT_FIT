<?php  
	require 'Console/Getopt.php';

	fwrite(STDOUT,"Start of parser.php(xgiesl00)".PHP_EOL."");

	
	
	
	
Class Main
{
	#xml
	private $xml;
	private $number = 0;
	#arrays with instructions
	private $zero_op_instructions 	= array('CREATEFRAME','PUSHFRAME','POPFRAME','RETURN','BREAK');
	private $one_op_instructions 	= array('DEFVAR','CALL','PUSHS','POPS','WRITE','LABEL','JUMP','EXIT','DPRINT');
	private $two_op_instructions 	= array('MOVE','INT2CHAR','READ','STRLEN','TYPE',);
	private $three_op_instructions 	= array('ADD','SUB','MUL','IDIV','LT','GT','EQ','AND','OR','NOT','STR2INT','CONCAT','GETCHAR','SETCHAR','JUMPIFEQ','JUMPIFNEQ');


	function print_help()
	{
		$help = "HELP OF THE PROGRAM.".PHP_EOL;
		$help .= "neco dalsiho".PHP_EOL;

		echo $help;
	}

	function get_opts()
	{
    	$short_opts = "s::";
    	$long_opts = array("help::","source::");
    	$options = getopt($short_opts, $long_opts);
    	//var_dump($options);
    	return $options;
	}

	function parse_parameters()
	{
		$args = $this->get_opts();
		#--help
		if(array_key_exists('help', $args))
			{
				if(count($args) != 1)
				{
					fwrite(STDERR, "ERROR WITH PARAMETERS.".PHP_EOL);
					exit(10);
				}
				$this->print_help();
				exit(0);
			}
		#only --source
		elseif (array_key_exists('source', $args) && ( array_key_exists('s', $args) == false)) 
		{
			$this->parse_file($args['source']);
		}
		#only -s
		elseif (array_key_exists('s', $args) && ( array_key_exists('source', $args) == false)) 
		{
			$this->parse_file($args['s']);
		}
		#no -s or --source
		elseif ((array_key_exists('s', $args) == false) && ( array_key_exists('source', $args) == false)) 
		{
			#TODO
			$this->parse_file(2);
		}
		else
		{
			fwrite(STDERR,"ERROR WITH PARAMETERS".PHP_EOL);
			exit(10);
		}
	}

	function remove_commentars($line)
	{
		$line = explode('#', $line);
		return rtrim($line[0],"\n");
	}

	function process_instruction($instruction,$args)
	{
		#var_dump($instruction);
		echo "count = ".count($instruction).PHP_EOL."args = ".$args.PHP_EOL;
		if(count($instruction)-1 != $args)
		{
			fwrite(STDERR,"BAD ARGUMENTS FOR OPERATION");
			exit(23);
		}

		$xml_element = $this->add_instruction($instruction[0]);
		for ($i = 0;$i < $args;$i++)
		{
			
			$arg = explode('@',$instruction[$i+1]);
			if(strcmp($arg[0],'GF')||strcmp($arg[0],'TF')||strcmp($arg[0],'LF'))
			{
				$arg_type = 'var';
			}
			else
			{
				$arg_type = $arg[0];
			}
			$arg_val = $instruction[$i+1];
			$this->add_argument($xml_element,$i,$arg_type,$arg_val);
		}
	}

	function add_instruction($opcode)
	{
		$instruction = $this->xml->addChild('instruction' .' order="'.$this->$number.'"'.' opcode="'.$opcode.'"');
		$this->$number++;

		return $instruction;
	}

	function add_argument($parent,$number,$type,$value)
	{
		$parent->addChild('arg'.$number.' type='.$type,$value);
	}

	function process_line($line)
	{
		$line = $this->remove_commentars($line);

		if(!is_null($line))
		{
			#echo "Processing line: ".$line.":".PHP_EOL;
			#echo "LINE:".substr($line, -1).":".PHP_EOL;
			#echo "LINE:".rtrim($line).":".PHP_EOL;

			$words = explode(' ',rtrim($line));
			

			if(!is_null($words[0]))
			{
				echo "Processing: ".$words[0].PHP_EOL;
				if($this->$number == 0)
				{	
					#echo "words[0] = " .$words[0].PHP_EOL;
					if(strcmp($words[0],'.IPPcode19'))
					{
						fwrite(STDERR, "No .IPPcode19 on first line\n");
						exit(21);
					}
					$this->$number++; 
		
				}elseif (in_array($words[0],$this->zero_op_instructions))
				{
					echo "ZERO OP INSTURCTION:".$instruction.PHP_EOL;
					$this->process_instruction($words,0);

				}elseif (in_array($words[0],$this->one_op_instructions))
				{
					echo "ONE OP INSTURCTION:".$instruction.PHP_EOL;
					$this->process_instruction($words,1);

				}elseif (in_array($words[0],$this->two_op_instructions))
				{
					echo "TWO OP INSTURCTION:".$instruction.PHP_EOL;
					$this->process_instruction($words,2);

				}elseif (in_array($words[0],$this->three_op_instructions))
				{
					echo "THREE OP INSTURCTION:".$instruction.PHP_EOL;
					$this->process_instruction($words,3);

				}
				else 
				{
					fwrite(STDERR,"BAD INSTRUCTION");
					exit(53);
				}
				


			}#isnull $words
		}#isnull $line
		echo PHP_EOL."LINE PROCCESSED".PHP_EOL;
		
	}#process_line

	function parse_file($file)
	{
		fwrite(STDOUT,"Opening file: " . $file .PHP_EOL);

		if($file != 2)
		{
			try{
				$file = fopen($file, "r");
			}
			catch (Exception $e) 
			{
    			echo 'Caught exception: ',  $e->getMessage(), "\n";
    			exit(11);
			}
		}
		else
		{
			echo "opening STDIN".PHP_EOL;
			$file = fopen('php://stdin', 'r');
		}	
		$this->xml = new SimpleXMLElement('<?xml version="1.0" encoding="UTF-8"?>'.'<program language="IPPcode19">'.'</program>');
		
		#$this->xml->addChild('program','');

		#read line by line
		while (($line = fgets($file)) !== false)
		{
			$this->process_line($line,$this->xml,$this->$number);
		}


		fclose($file);

		print($this->xml->asXML());

		exit(0);	
	}

	function run()
	{

		$this->parse_parameters();

	}

}


##running 
$main = new Main;
$main->run();


?>
