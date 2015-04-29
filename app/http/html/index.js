$(function(){

	var scanning = 0;
	
	var scanResult=null;

	function wifiConnectModal(id){

		if(scanResult==null)
			return;

		var modal = $('#connect-modal');
		var ap = scanResult.ap[id];

		modal.find('#modal-connect-title').text('Connect to '+ap.ssid);
		if(ap.enc==0){
			modal.find('#modal-connect-password').hide();
			modal.find('#modal-connect-no-password').show();
		}
		else{
			modal.find('#modal-connect-password').show();
			modal.find('#modal-connect-no-password').hide();

			modal.find('#modal-connect-password-input').val('');
		}

		modal.data('ap',ap);

		$('#connect-modal .modal-content.connect').show();
		$('#connect-modal .modal-content.success').hide();
		$('#connect-modal .modal-content.connecting').hide();
		
		modal.modal({show:true,backdrop:false});

		$('#modal-connect-password-input').focus();
	}

	function wifiScan(){

		scanning=1;
		//hide result, show progress
		$('#scan-result').hide();
		$('#scan-progress').show();

		$.ajax(
		 	{
				type:'POST',
				url:'/api/wifi/scan',
				data:null,				
				success: function(data){

					scanning=0;

					scanResult = data;

					//hide progress, show result
					$('#scan-result').show();
					$('#scan-progress').hide();

					if(data.ap_count==0){
						$('#scan-result-empty').show();
						$('#scan-result-table').hide();
					}
					else{

						$('#scan-result-empty').hide();
						$('#scan-result-table').show();

						$('#scan-result tbody tr').remove(); //empty table

						//sort 
						data.ap = data.ap.sort(function(a,b){return b.rssi - a.rssi });

						for(i=0;i<data.ap_count;i++){
			       			
			       			var ap = data.ap[i];
			       			var tr = $('<tr>');
			       			tr.data('ap-id',i);
				          	
				          	var td_name = $('<td>');
				          	td_name.text(ap.ssid);
				          	td_name.appendTo(tr);

				          	var td_channel = $('<td class="text-center">');
				          	td_channel.text(ap.channel);
				          	//td_channel.appendTo(tr);

				          	var td_enc = $('<td class="text-center">')
				          	tr.data('enc',ap.enc);
				          	switch(ap.enc){
				          		case 0:
				          			td_enc.text("OPEN");
				          			break;
				          		case 1:
				          			td_enc.text("CLOSED");
				          			break;
				          		case 2:
				          			td_enc.text("CLOSED");
				          			break;
				          		case 3:
				          			td_enc.text("CLOSED");
				          			break;
				          		case 4:
				          			td_enc.text("CLOSED");
				          			break;
				          		default:
				          			td_enc.text("CLOSED");
				          			break;
				          	}
				          	td_enc.appendTo(tr);

				          	var td_signal = $('<td  class="text-center">');
				          	var signal_pct = 100*ap.rssi/256.0;
				          	var signal_span = $('<span class="label"></span>');
				          	signal_span.text(signal_pct.toFixed('f2')+'%');

				          	if(signal_pct > 70) signal_span.addClass('label-success');
				          	else if(signal_pct > 40) signal_span.addClass('label-warning');
				          	else signal_pct.addClass('label-danger');

				          	signal_span.appendTo(td_signal);
				          	td_signal.appendTo(tr);

				          	tr.appendTo($('#scan-result tbody').first());

				           	
				        }
				    }
				},
				fail:function(){

					scanning=0;
					//hide progress, show result
					$('#scan-result').show();
					$('#scan-progress').hide();

					$('#scan-result-empty').show();
					$('#scan-result-table').hide();

				}
			});			
	}

	function wifiConnect(ssid,pwd){

		var postData = {"ssid":ssid,"pwd":pwd};

		$('#connect-modal .modal-content.connect').hide();
		$('#connect-modal .modal-content.success').hide();
		$('#connect-modal .modal-content.connecting').show();

		$.ajax(
		 	{
				type:'POST',
				url:'/api/wifi/connect',
				data:JSON.stringify(postData),
				contentType: 'application/json',
				success:function(data){

					$('#connect-modal .modal-content.connect').hide();
					$('#connect-modal .modal-content.success').show();
					$('#connect-modal .modal-content.connecting').hide();

					$('#connect-modal .modal-content.success h3')
						.text('Relay now connected to '+ssid+' with ip: '+data.ip);
						
					wifiStatus();

				},
				fail:function(){

					$('#connect-modal .modal-content.connect').show();
					$('#connect-modal .modal-content.success').hide();
					$('#connect-modal .modal-content.connecting').hide();
					wifiStatus();
				}
			});
	}

	function checkInternet(){

		var status_internet=$('#status-internet');
		status_internet.text("Checking...");

		$.ajax(
		 	{
				type:'POST',
				url:'/api/wifi/checkInternet',
				data:'',
				contentType: 'application/json',
				success:function(data){

					if(data.status==1){
						status_internet.text("YES!");
					}
					else{
						status_internet.text("NO :(");
					}

				}
			});

	}

	function wifiStatus(){

		$.ajax(
		 	{
				type:'POST',
				url:'/api/wifi/status',
				data:'',
				contentType: 'application/json',
				success:function(data){

					var status_status=$('#status-status');
					var status_ip = $('#status-ip');
					var status_ssid = $('#status-ssid');
					switch(data.station_status){
					 	case 5:
					 		status_status.text('Connected');					 		
					 		break;
					 	case 4:
					 		status_status.text('Fail');
					 		break;
				 		case 3:
					 		status_status.text('Ap Not Found');
					 		break;
					 	case 2:
					 		status_status.text('Wrong Password');
					 		break;
					 	case 1:
					 		status_status.text('Connecting');
					 		break;
					 	case 0:
					 		status_status.text('Not Configured');
					 		break;
					}
					status_ssid.text(data.ssid);	
					status_ip.text(data.ip);

					if(data.station_status==5){

						$('#btn-disconnect').show();
						$('#network-scan').hide();
						checkInternet();
					}
					else{
						wifiScan();
						$('#network-scan').show();
						$('#btn-disconnect').hide();
					}

				}
			});

	}

	function wifiDisconnect(){

		$.ajax(
		 	{
				type:'POST',
				url:'/api/wifi/disconnect',
				data:'',
				contentType: 'application/json',
				success:function(data){

					wifiStatus();

				}
			});

		var status_internet=$('#status-internet');
		status_internet.text("");

	}

	function relayQueryStatus(){

		$.ajax(
		 	{
				type:'POST',
				url:'/api/relay/state',
				data:'',
				contentType: 'application/json',
				success:function(data){

					$.each(data.relays,function(index,item){

						var btn = $('#relay-button-'+item.relay);

						relayStatus(item.relay,item.state);

					});										
					

				}
			});

	}

	function relayStatus(id,status){

		var btn = $('#relay-button-'+id);

		if(status==0){
			btn.text('Relay #'+id+' OFF');
			btn.removeClass('btn-success');
			btn.addClass('btn-default');
		}
		else{
			btn.text('Relay #'+id+' ON');
			btn.removeClass('btn-default');
			btn.addClass('btn-success');
			
		}
	}

	function relayToggle(id){

		var data = {'relay':id}
		$.ajax(
		 	{
				type:'POST',
				url:'/api/relay/toggle',
				data:JSON.stringify(data),
				contentType: 'application/json',
				success:function(data){

					relayStatus(id,data.state);

				}
			});

	}
	//sensor
	var canvas = $('#sensor-chart');
	canvas.attr('width',canvas.parent().width());
	var ctx = document.getElementById("sensor-chart").getContext("2d");			
	var options = {animation :false} ;
	var chartData = {
		    labels: [],
		    datasets: [
				        {
				            label: "Temperature",
				            fillColor: "rgba(220,220,220,0.2)",
				            strokeColor: "rgba(220,220,220,1)",
				            pointColor: "rgba(220,220,220,1)",
				            pointStrokeColor: "#fff",
				            pointHighlightFill: "#fff",
				            pointHighlightStroke: "rgba(220,220,220,1)",
				            data: []
				        },
				        {
				            label: "Humidity",
				            fillColor: "rgba(151,187,205,0.2)",
				            strokeColor: "rgba(151,187,205,1)",
				            pointColor: "rgba(151,187,205,1)",
				            pointStrokeColor: "#fff",
				            pointHighlightFill: "#fff",
				            pointHighlightStroke: "rgba(151,187,205,1)",
				            data: []
				        }
		    		]
			};

	//init data			

	var lineChart = new Chart(ctx).Line(chartData, options);

	function sensorRead(){

		$.ajax(
		 	{
				type:'POST',
				url:'/api/dht/read',
				data:'',
				contentType: 'application/json',
				success:function(data){

					$('#temp-data').text(data.temp.toFixed(2)+' C');
					$('#hum-data').text(data.hum.toFixed(2)+' %');
					//chart push new
			        lineChart.addData([data.temp,data.hum],'');
				}
			});

	}	
	setInterval(sensorRead,5000); //read every 5s
	

	$('#btn-scan').on('click',function(e){
		e.preventDefault();
		e.stopPropagation();

		if(!scanning)
			wifiScan();
	});

	$('#btn-disconnect').on('click',function(e){
		e.preventDefault();
		e.stopPropagation();

		wifiDisconnect();
	});
	
	$(document).on('click','#scan-result tbody tr',function(e){

		e.preventDefault();
		e.stopPropagation();

		var id = $(e.currentTarget).data('ap-id');
		wifiConnectModal(id);

	});

	$('a.relay-button').on('click',function(e){
		e.preventDefault();
		e.stopPropagation();
		var id = $(this).data('relay');
		relayToggle(id);
	});

	$(document).on('click','#modal-connect-button-connect',function(e){

		e.preventDefault();
		e.stopPropagation();

		var modal = $('#connect-modal');
		var ap = modal.data('ap');

		var pwd = modal.find('#modal-connect-password-input').val();

		wifiConnect(ap.ssid,pwd);

	});		

	wifiStatus();
	relayQueryStatus();

});

		