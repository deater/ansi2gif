program fontprint;

{by vince weaver... Prints out text file in VGA fonts}
{modif: Jiri Kuchta;   kuchta@fee.vutbr.cz }
{Translated to Linux by Vince Weaver 8 October 1996;  vmweaver@wam.umd.edu}
{Compile with FPK-Pascal}
{http://sun01.brain.uni-freiburg.de/~klaus/pascal/fpk-pas}
{}
{Official Page:  http://www.glue.umd.edu/~weave/vmwprod/vmwsoft.html}

var buff:array[0..1] of byte;
    f:file;
    f2,lst:text;
    parx:string;
    i,yy,arraysize2:integer;
    out_file,tf,pstring:string;
    font1:array[0..4096] of byte;
    outputar:array[0..720,0..15] of byte;
    fonty:integer;
    stringtoout:string;
    po:integer;
    zz,tempbyte,tempbyte2,tempbyte3:byte;
    d0,d1,d2,d3,d4,d5,d6,d7:byte;
    stlen,bb1,bb2,arraysize:byte;
    UsingAnEpson,ExtraSpacingOn,ExtraWide:boolean;
    ExtraWideCh:char;

procedure outputline(st:string);

begin
  if ExtraSpacingOn then arraysize:=9
                    else arraysize:=8;
  stlen:=length(st);
  for po:=1 to stlen do begin
      tempbyte3:=ord(st[po]);
      for zz:=0 to 15 do begin
          tempbyte:=font1[((tempbyte3)*16)+zz];
          d0:=tempbyte div 128;
          Tempbyte2:=tempbyte mod 128;
          d1:=tempbyte2 div 64;
          tempbyte2:=tempbyte2 mod 64;
          d2:=tempbyte2 div 32;
          tempbyte2:=tempbyte2 mod 32;
          d3:=tempbyte2 div 16;
          tempbyte2:=tempbyte2 mod 16;
          d4:=tempbyte2 div 8;
          tempbyte2:=tempbyte2 mod 8;
          d5:=tempbyte2 div 4;
          tempbyte2:=tempbyte2 mod 4;
          d6:=tempbyte2 div 2;
          d7:=tempbyte2 mod 2;
          outputar[((po-1)*arraysize),zz]:=d0;
          outputar[((po-1)*arraysize)+1,zz]:=d1;
          outputar[((po-1)*arraysize)+2,zz]:=d2;
          outputar[((po-1)*arraysize)+3,zz]:=d3;
          outputar[((po-1)*arraysize)+4,zz]:=d4;
          outputar[((po-1)*arraysize)+5,zz]:=d5;
          outputar[((po-1)*arraysize)+6,zz]:=d6;
          outputar[((po-1)*arraysize)+7,zz]:=d7;
          if ExtraSpacingOn then outputar[((po-1)*arraysize)+8,zz]:=0;
     end;
  end;
  if ExtraSpacingOn then arraysize2:=720
                    else arraysize2:=640;
  for i:=0 to arraysize2 do outputar[i,0]:=outputar[i,0]*128;
  for i:=0 to arraysize2 do outputar[i,1]:=outputar[i,1]*64;
  for i:=0 to arraysize2 do outputar[i,2]:=outputar[i,2]*32;
  for i:=0 to arraysize2 do outputar[i,3]:=outputar[i,3]*16;
  for i:=0 to arraysize2 do outputar[i,4]:=outputar[i,4]*8;
  for i:=0 to arraysize2 do outputar[i,5]:=outputar[i,5]*4;
  for i:=0 to arraysize2 do outputar[i,6]:=outputar[i,6]*2;

  for i:=0 to arraysize2 do outputar[i,8]:=outputar[i,8]*128;
  for i:=0 to arraysize2 do outputar[i,9]:=outputar[i,9]*64;
  for i:=0 to arraysize2 do outputar[i,10]:=outputar[i,10]*32;
  for i:=0 to arraysize2 do outputar[i,11]:=outputar[i,11]*16;
  for i:=0 to arraysize2 do outputar[i,12]:=outputar[i,12]*8;
  for i:=0 to arraysize2 do outputar[i,13]:=outputar[i,13]*4;
  for i:=0 to arraysize2 do outputar[i,14]:=outputar[i,14]*2;

  bb2:=(stlen*arraysize) div 256;
  bb1:=(stlen*arraysize) mod 256;

  write(lst,#27,'3',#24);
  if UsingAnEpson then write(lst,#27,ExtraWideCh,chr(bb1),chr(bb2))
                   else write(lst,#27,'[','g',chr(bb1),chr(bb2),chr(1));
  for yy:=0 to (arraysize*stlen)-1 do
      write(lst,chr(outputar[yy,0]+outputar[yy,1]+outputar[yy,2]+
                    outputar[yy,3]+outputar[yy,4]+outputar[yy,5]+
                    outputar[yy,6]+outputar[yy,7]));
  write(lst,#10,#13);

   if UsingAnEpson then write(lst,#27,ExtraWideCh,chr(bb1),chr(bb2))
                   else write(lst,#27,'[','g',chr(bb1),chr(bb2),chr(1));

  for yy:=0 to (arraysize*stlen)-1 do
      write(lst,chr(outputar[yy,8]+outputar[yy,9]+outputar[yy,10]+
                    outputar[yy,11]+outputar[yy,12]+outputar[yy,13]+
                    outputar[yy,14]+outputar[yy,15]));

  write(lst,#10,#13);
end;


begin
  ExtraWide:=false;
  ExtraSpacingOn:=true;
  UsingAnEpson:=true;
  pstring:='';
  tf:='';
  out_file:='';
  if paramcount<>0 then begin
     for i:=1 to paramcount do
     begin
        parx:=paramstr(i);
        if parx='?' then insert('/',parx,1);
        if (parx[1]='/') or (parx[1]='-') then
        begin
         delete(parx,1,1);
         if parx='0' then ExtraSpacingOn:=false;
         if parx='1' then ExtraWide:=true;
         if (parx='?') or (parx='h') or
            (parx='H') or (parx='?') then
         begin
            writeln;
	    writeln;
            writeln('FontPrint v 2.5.1 Paramaters Supported:');
	    writeln;
            writeln('   fontprint [/0] [/1] [font_file] [text_file] [output_file]');
            writeln;
            writeln('      /0  eliminates the extra space added between each character for clarity');
            writeln('      /1  prints in wide mode (less clear)');
            writeln;
	    writeln('      font_file:   any standard VGA font, usually 4096 bytes');
	    writeln('      text_file:   the file you wish to print');
	    writeln('      output_file: the file where the output is placed. (defaults to fprint.out)');
	    writeln('                   This is the file to send to your printer.'); 
	    writeln('                   (e.g. cat fprint.out > /dev/lp1   )'); 
            writeln;
	    halt(1);
         end;
        end else
        begin
           if pstring='' then pstring:=parx else
              if tf='' then tf:=parx else
	         if out_file='' then out_file:=parx;
	   
        end;
     end;
  end;
  if ExtraWide then ExtraWideCh:='K'
               else ExtraWideCh:='L';
  writeln;
  writeln;
  writeln('The amazing VGA font printer!  by Vince Weaver, vmweaver@wam.umd.edu');
  writeln('                               New Linux Port, v 2.5.1');
  writeln('                               fontprint /?        displays help');
  writeln;
  write('Font file to use: ');
  if pstring='' then begin readln(pstring); writeln; end else writeln(pstring);

  assign(f,pstring);
  {$I-}
   reset(f,1);
  {$I+}
   if Ioresult<>0 then begin
      writeln('File ',pstring,' not found.');
      writeln;
      halt(2);
   end;

  fonty:=0;
  while (not eof(F)) and (fonty<4096) do
     begin
       blockread(f,buff,1);
       font1[fonty]:=buff[0];
       inc(fonty);
     end;
  close(f);

  write('Text file to print: ');
  if tf='' then  begin readln(tf); writeln; end else writeln(tf);

  assign(f2,tf);
  {$I-}
    reset(f2);
  {$I+}
  if Ioresult<>0 then begin
     writeln('File ',tf,' not found.');
     writeln;
     halt(3);
  end;
  
  write('File to put output into? [default="fprint.out"]:');
  if out_file='' then begin readln(out_file); end else writeln(out_file);
  assign(lst,out_file);
  {$I-}
    rewrite(lst);
   {$I+}
   If ioresult<>0 then begin
      writeln('Cannot open file ',out_file);
      writeln;
      halt(4);
   end;
  
{
 }

  while not eof(f2) do begin
     readln(f2,stringtoout);
     outputline(stringtoout);
  end;
  close(f2);
  close(lst);
  writeln;
  writeln('Completed. To print, just send file ',out_file,' to the printer.');
  writeln('           (e.g.  cat ',out_file,' > /dev/lp1   )');
  writeln;
end.
