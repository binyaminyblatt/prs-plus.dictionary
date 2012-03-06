package org.kartu.dict.xdxf.visual.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlValue;

/**
 * Corresponds to "tr" tag (transcription) in xdxf visual format
 * 
 * @author kartu
 */
@XmlRootElement (name="tr")
public class TR {
	@XmlValue
	public String value;
	
	@Override
	public String toString() {
		return " [" + value + "]\n";
	}
}
