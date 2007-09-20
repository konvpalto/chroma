// $Id: inline_plaquette.cc,v 3.8 2007-09-20 19:16:57 edwards Exp $
/*! \file
 *  \brief Inline plaquette
 */

#include "meas/inline/glue/inline_plaquette.h"
#include "meas/inline/abs_inline_measurement_factory.h"
#include "meas/inline/make_xml_file.h"
#include "meas/glue/mesplq.h"
#include "meas/inline/io/named_objmap.h"

#include "meas/inline/io/default_gauge_field.h"

#include "actions/gauge/gaugestates/gauge_createstate_factory.h"
#include "actions/gauge/gaugestates/gauge_createstate_aggregate.h"

namespace Chroma 
{ 

  namespace InlinePlaquetteEnv 
  { 
    namespace
    {
      AbsInlineMeasurement* createMeasurement(XMLReader& xml_in, 
					      const std::string& path) 
      {
	AbsInlineMeasurement* p = new InlinePlaquette(InlinePlaquetteParams(xml_in, path));
	if (0 == p)
	{
	  // This might happen on any node, so report it
	  cerr << name << ": error creating InlinePlaquette in " << __func__ << endl;
	  QDP_abort(1);
	}
	return p;
      }

      //! Local registration flag
      bool registered = false;
    }

    const std::string name = "PLAQUETTE";

    //! Register all the factories
    bool registerAll() 
    {
      bool success = true; 
      if (! registered)
      {
	success &= CreateGaugeStateEnv::registerAll();
	success &= TheInlineMeasurementFactory::Instance().registerObject(name, createMeasurement);
	registered = true;
      }
      return success;
    }

  }


  //! Plaquette input
  void read(XMLReader& xml, const string& path, InlinePlaquetteParams::Param_t& param)
  {
    XMLReader paramtop(xml, path);

    int version;
    read(paramtop, "version", version);
    param.cgs = CreateGaugeStateEnv::nullXMLGroup();

    switch (version) 
    {
    case 2:
      if (paramtop.count("GaugeState") != 0)
	param.cgs = readXMLGroup(paramtop, "GaugeState", "Name");
      break;

    default:
      QDPIO::cerr << "InlinePlaquetteParams::Param_t: " << version 
		  << " unsupported." << endl;
      QDP_abort(1);
    }
  }

  //! Plaquette output
  void write(XMLWriter& xml, const string& path, const InlinePlaquetteParams::Param_t& param)
  {
    push(xml, path);

    int version = 2;
    write(xml, "version", version);
    xml << param.cgs.xml;

    pop(xml);
  }


  //! Plaquette input
  void read(XMLReader& xml, const string& path, InlinePlaquetteParams::NamedObject_t& input)
  {
    XMLReader inputtop(xml, path);

    read(inputtop, "gauge_id", input.gauge_id);
  }

  //! Plaquette output
  void write(XMLWriter& xml, const string& path, const InlinePlaquetteParams::NamedObject_t& input)
  {
    push(xml, path);

    write(xml, "gauge_id", input.gauge_id);

    pop(xml);
  }


  // Params
  InlinePlaquetteParams::InlinePlaquetteParams() 
  { 
    frequency = 0; 
    param.cgs          = CreateGaugeStateEnv::nullXMLGroup();
    named_obj.gauge_id = InlineDefaultGaugeField::getId();
    xml_file ="";
  }

  InlinePlaquetteParams::InlinePlaquetteParams(XMLReader& xml_in, const std::string& path) 
  {
    try 
    {
      XMLReader paramtop(xml_in, path);

      if (paramtop.count("Frequency") == 1)
	read(paramtop, "Frequency", frequency);
      else
	frequency = 1;

      // Params
      read(paramtop, "Param", param);

      // Ids
      read(paramtop, "NamedObject", named_obj);

      // Possible alternate XML file pattern
      if (paramtop.count("xml_file") != 0) {
	read(paramtop, "xml_file", xml_file);
      }
    }
    catch(const std::string& e) 
    {
      QDPIO::cerr << "Caught Exception reading XML: " << e << endl;
      QDP_abort(1);
    }
  }


  void 
  InlinePlaquette::operator()(unsigned long update_no,
			      XMLWriter& xml_out) 
  {
    if( params.xml_file != "" ) {
      string xml_file = makeXMLFileName(params.xml_file, update_no);
      push( xml_out, "Plaquette");
      write(xml_out, "update_no", update_no);
      write(xml_out, "xml_file", xml_file);
      pop(xml_out);

      XMLFileWriter xml(xml_file);
      func(update_no, xml);
    }
    else {
      func(update_no, xml_out);
    }

  }

  void 
  InlinePlaquette::func(const unsigned long update_no, 
			XMLWriter& xml_out) 
  {
    QDPIO::cout << InlinePlaquetteEnv::name << ": entering" << endl;

    START_CODE();
    
    // Test and grab a reference to the gauge field
    multi1d<LatticeColorMatrix> u;
    XMLBufferWriter gauge_xml;

    try
    {
      u = TheNamedObjMap::Instance().getData< multi1d<LatticeColorMatrix> >(params.named_obj.gauge_id);
      TheNamedObjMap::Instance().get(params.named_obj.gauge_id).getRecordXML(gauge_xml);

      // Set the construct state and modify the fields
      {
	std::istringstream  xml_s(params.param.cgs.xml);
	XMLReader  gaugetop(xml_s);

	QDPIO::cout << InlinePlaquetteEnv::name << ": create the CreateGaugeState" << endl;
	Handle<CreateGaugeState< multi1d<LatticeColorMatrix>, multi1d<LatticeColorMatrix> > > 
	  cgs(TheCreateGaugeStateFactory::Instance().createObject(params.param.cgs.id,
								  gaugetop,
								  params.param.cgs.path));

	// Additional paranoia
	if (0 == *cgs)
	{
	  // This might happen on any node, so report it
	  cerr << InlinePlaquetteEnv::name << ": error - the cgs is null in " << __func__ << endl;
	  QDP_abort(1);
	}

	QDPIO::cout << InlinePlaquetteEnv::name << ": apply createGaugeState" << endl;
	Handle<GaugeState< multi1d<LatticeColorMatrix>, multi1d<LatticeColorMatrix> > > 
	  state((*cgs)(u));

	// Pull the u fields back out from the state since they might have been
	// munged with gaugeBC's
	QDPIO::cout << InlinePlaquetteEnv::name << ": get the links" << endl;
	u = state->getLinks();
      }
    }
    catch( std::bad_cast ) 
    {
      QDPIO::cerr << InlinePlaquetteEnv::name << ": caught dynamic cast error" 
		  << endl;
      QDP_abort(1);
    }
    catch (const string& e) 
    {
      QDPIO::cerr << InlinePlaquetteEnv::name << ": map call failed: " << e 
		  << endl;
      QDP_abort(1);
    }

    QDPIO::cout << InlinePlaquetteEnv::name << ": compute plaquette" << endl;
    push(xml_out, "Plaquette");
    write(xml_out, "update_no", update_no);

    Double w_plaq, s_plaq, t_plaq, link; 
    multi2d<Double> plane_plaq;

    MesPlq(u, w_plaq, s_plaq, t_plaq, plane_plaq, link);
    write(xml_out, "w_plaq", w_plaq);
    write(xml_out, "s_plaq", s_plaq);
    write(xml_out, "t_plaq", t_plaq);

    if (Nd >= 2)
    {
      write(xml_out, "plane_01_plaq", plane_plaq[0][1]);
    }

    if (Nd >= 3)
    {
      write(xml_out, "plane_02_plaq", plane_plaq[0][2]);
      write(xml_out, "plane_12_plaq", plane_plaq[1][2]);
    }

    if (Nd >= 4)
    {
      write(xml_out, "plane_03_plaq", plane_plaq[0][3]);
      write(xml_out, "plane_13_plaq", plane_plaq[1][3]);
      write(xml_out, "plane_23_plaq", plane_plaq[2][3]);
    }

    write(xml_out, "link", link);
    
    pop(xml_out); // pop("Plaquette");
    
    END_CODE();

    QDPIO::cout << InlinePlaquetteEnv::name << ": exiting" << endl;
  } 

}
