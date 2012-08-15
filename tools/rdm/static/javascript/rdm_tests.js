/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Copyright (C) 2012 Ravindra Nath Kakarla
 */


/**
 * RDMTests class
 */
RDMTests = function() {
};


rdmtests = new RDMTests();


/**
 * AJAX loader image
 * @this {RDMTests}
 */
RDMTests.ajax_loader = "<img src='/static/images/loader.gif' />";


/**
 * Maintains a list of all the tests along with their states, categories,
 * definitions with keys sorted in the order they are run.
 * @this {RDMTests}
 */
RDMTests.TEST_RESULTS = new Array();


/**
 * Prepares the notification div and displays it on the page.
 * @this {RDMTests}
 * @param {Objec} options An object containing title and message to be
 * displayed.
 *
 *  {
 *    'title': 'Title to display on notification',
 *    'message': 'Notification Message',
 *    'is_dismissable': true
 *  }
 */
RDMTests.prototype.set_notification = function(options) {
  if (options.title != undefined || options.title != null) {
    $('#rdm-tests-notification-title').html(options.title);
  }
  if (options.message != undefined || options.message != null) {
    $('#rdm-tests-notification-message').html(options.message);
  }
  if (options.is_dismissable != undefined || options.is_dismissable != null) {
    if (options.is_dismissable == true) {
      $('#rdm-tests-notification-button').show();
    }
  }
  $('#rdm-tests-notification').show();
};


/**
 * Clears the notification text and hides it from page.
 * @this {RDMTests}
 */
RDMTests.prototype.clear_notification = function() {
  $('#rdm-tests-notification-title').empty();
  $('#rdm-tests-notification-message').empty();
  $('#rdm-tests-notification-button').hide();
  $('#rdm-tests-notification').hide();
};


/**
 * Binds click, keypress and other events to DOMs and
 * triggers the their respective handlers.
 * @this {RDMTests}
 */
RDMTests.prototype.bind_events_to_doms = function() {
  $('#universe_options').change(function() {
    rdmtests.update_device_list();
  });

  $('#rdm-discovery-button').click(function() {
    rdmtests.run_discovery();
  });

  $('#rdm-tests-notification-button').click(function() {
    rdmtests.clear_notification();
  });

  $('#rdm-tests-selection-run_tests').click(function() {
    rdmtests.validate_form();
  });

  $('#rdm-tests-send_dmx_in_bg').change(function() {
    $('#rdm-tests-dmx_options').toggle('fast');
  });

  $(document).keydown(function(e) {
    var key = e.keyCode || e.which;
    var results_div = $('#rdm-tests-results');
    var test_frame = $('#tests_control_frame');

    if (results_div.css('display') == 'block' && key == 27) {
      results_div.hide();
      $('#tests_control_frame').show();
    }

    if (key == 13 && test_frame.css('display') == 'block') {
      rdmtests.validate_form();
    }
  });

  $('#rdm-tests-results-button-dismiss').click(function() {
    $('#rdm-tests-results').hide();
    $('#tests_control_frame').show();
  });

  $('#rdm-tests-results-button-run_again').click(function() {
    rdmtests.validate_form();
  });

  $('#rdm-tests-results-button-download').click(function() {
    var uid = $('#devices_list').val();
    var timestamp = RDMTests.timestamp;
    $('#rdm-tests-download').attr('src',
        '/DownloadResults?uid=' + uid +
        '&timestamp=' + timestamp);
  });

  $.each([
    '#rdm-tests-results-summary-by_catg',
    '#rdm-tests-results-warnings',
    '#rdm-tests-results-advisories'
  ], function(i, div) {
    $(div).find('legend').click(function() {
      $(div).find('legend')
      .toggleClass('ola-expander-collapsed ola-expander-expanded');
      $(div).find('div').toggle();
    });
  });

  $.each([
    $('#rdm-tests-results-summary-filter-by_catg'),
    $('#rdm-tests-results-summary-filter-by_state')
  ], function(i, div) {
    $(div).change(function() {
      rdmtests.filter_results($('#rdm-tests-results-list'), {
        'category': $('#rdm-tests-results-summary-filter-by_catg').val(),
        'state': $('#rdm-tests-results-summary-filter-by_state').val()
      });
    });
  });
};


/**
 * Prepares a list item with appropriate color, definition and value.
 * This will be appended to the results list.
 * @this {RDMTests}
 * @param {String} definition Key (or test definition) in TEST_RESULTS object.
 * @return {Object} A jQuery object representation for a prepared list item.
 */
RDMTests.prototype.make_results_list_item = function(definition) {
  var test_option = $('<option />').val(definition).text(definition);
  rdmtests.add_state_class(
                           RDMTests.TEST_RESULTS[definition]['state'],
                           test_option);
  return test_option;
};


/**
 * Filters definitions in results list by category and state of test result.
 * @this {RDMTests}
 * @param {Object} results_dom A jQuery object representation of HTML <ul>
 * which holds test result definitions.
 * @param {Object} filter_options An Object containing selected filter options.
 */
RDMTests.prototype.filter_results = function(results_dom, filter_options) {
  $(results_dom).html('');
  var filter_category = filter_options['category'];
  var filter_state = filter_options['state'];

  if (filter_category == 'All') {
    if (filter_state == 'All') {
      for (var definition in RDMTests.TEST_RESULTS) {
        $(results_dom).append(rdmtests.make_results_list_item(definition));
      }
    } else {
      for (definition in RDMTests.TEST_RESULTS) {
        if (RDMTests.TEST_RESULTS[definition]['state'] == filter_state) {
          $(results_dom).append(rdmtests.make_results_list_item(definition));
        }
      }
    }
  } else {
    if (filter_state == 'All') {
      for (definition in RDMTests.TEST_RESULTS) {
        if (RDMTests.TEST_RESULTS[definition]['category'] == filter_category) {
          $(results_dom).append(rdmtests.make_results_list_item(definition));
        }
      }
    } else {
      for (definition in RDMTests.TEST_RESULTS) {
        if (RDMTests.TEST_RESULTS[definition]['category'] == filter_category &&
            RDMTests.TEST_RESULTS[definition]['state'] == filter_state) {
          $(results_dom).append(rdmtests.make_results_list_item(definition));
        }
      }
    }
  }
};


/**
 * Sends an AJAX request to the RDM Tests Server and
 * triggers callback with the response.
 * Automatically displays ERROR notifications when a request fails.
 * @this {RDMTests}
 * @param {String} request Request string.
 * @param {Object} params An Object containing parameters to send to the server.
 * @param {Object} callback Handler to trigger.
 */
RDMTests.prototype.query_server = function(request, params, callback) {
  $.ajax({
    url: request,
    type: 'GET',
    data: params,
    dataType: 'json',
    success: function(data) {
      RDMTests.timestamp = data['timestamp'];
      if (data['status'] == true) {
        callback(data);
      } else {
        rdmtests.clear_notification();
        rdmtests.set_notification({
          'title': '[ERROR]',
          'message': data['message'],
          'is_dismissable': true
        });
        callback(data);
      }
    },
    cache: false
  });
};


/**
 * Updates the universe selector with available universes fetched
 * from RDM Tests Server.
 */
RDMTests.prototype.update_universe_list = function() {
  this.query_server('/GetUnivInfo', {}, function(data) {
    if (data['status'] == true) {
      var universes = data.universes;
      var universe_options = $('#universe_options');
      $('#universe_options').empty();
      $.each(universes, function(item) {
        universe_options.append($('<option />').val(universes[item]._id)
                                .text(universes[item]._name));
      });
      rdmtests.update_device_list();
    }
  });
};


/**
 * Triggers Full Discovery of devices via AJAX request
 * and updates the universe list.
 */
RDMTests.prototype.run_discovery = function() {
  rdmtests.set_notification({
    'title': 'Running Full Discovery',
    'message': RDMTests.ajax_loader
  });
  rdmtests.query_server('/RunDiscovery', {
    'u': $('#universe_options').val()}, function(data) {
    var devices_list = $('#devices_list');
    devices_list.empty();
    if (data['status'] == true) {
      var uids = data.uids;
      $.each(uids, function(item) {
        devices_list.append($('<option />').val(uids[item])
                              .text(uids[item]));
      });
      rdmtests.clear_notification();
    }
  });
};


/**
 * Updates the patched devices list for selected universe.
 */
RDMTests.prototype.update_device_list = function() {
  var universe_options = $('#universe_options');
  var devices_list = $('#devices_list');
  this.query_server('/GetDevices', {
    'u': universe_options.val() }, function(data) {
    if (data['status'] == true) {
      devices_list.empty();
      var uids = data.uids;
      $.each(uids, function(item) {
        devices_list.append($('<option />').val(uids[item]).text(uids[item]));
      });
    }
  });
};


/**
 * Fetches all Test Definitions available from the RDM Tests Server.
 * Initializes the multiselect widget with definitions.
 */
RDMTests.prototype.fetch_test_defs = function() {
  this.query_server('/GetTestDefs', {'c': 0}, function(data) {
    var tests_selector = $('#rdm-tests-selection-tests_list');
    var test_defs = data.test_defs;
    $.each(data.test_defs, function(item) {
      tests_selector.append($('<option />').val(test_defs[item])
                                           .text(test_defs[item]));
    });
    $('#rdm-tests-selection-tests_list').multiselect({
      sortable: false,
      searchable: true
    });
  });
};


/**
 * Triggers an AJAX call to run the tests with given Test Filter.
 * @param {Array} test_filter An array of tests to run.
 */
RDMTests.prototype.run_tests = function(test_filter) {
  this.set_notification({
    'title': 'Running ' + test_filter.length + ' tests',
    'message': RDMTests.ajax_loader
  });

  this.query_server('/RunTests', {
    'u': $('#universe_options').val(),
    'uid': $('#devices_list').val(),
    'w': $('#write_delay').val(),
    'f': ($('#rdm-tests-send_dmx_in_bg').attr('checked') ?
          $('#dmx_frame_rate').val() : 0),
    'c': $('#slot_count').val(),
    'c': ($('#rdm-tests-send_dmx_in_bg').attr('checked') ?
          $('#slot_count').val() : 128),
    't': test_filter.join(',')
  }, function(data) {
    if (data['status'] == true) {
      var failed_tests = $('#rdm-tests-selection-failed_tests');
      failed_tests.html('');
      var failed_defs = new Array();
      for (i in data['test_results']) {
        switch (data['test_results'][i]['state']) {
          case 'Failed':
            failed_defs.push(data['test_results'][i]['definition']);
            break;
        }
      }
      if ($(failed_tests).next().length > 0) {
        failed_tests.multiselect('destroy');
      }
      for (item in failed_defs) {
        failed_tests.append($('<option />')
                    .val(failed_defs[item])
                    .text(failed_defs[item]));
      }

      failed_tests.multiselect();
      rdmtests.clear_notification();
      rdmtests.display_results(data);
    } else {
      rdmtests.update_universe_list();
    }
  });
};


/**
 * Resets the test results screen by clearing all the DOMs contents
 * and hiding them.
 */
RDMTests.prototype.reset_results = function() {
  $.each(['#rdm-tests-results-uid',
    '#rdm-tests-results-stats-figures',
    '#rdm-tests-results-summary-filter-by_catg',
    '#rdm-tests-results-warnings-content',
    '#rdm-tests-results-advisories-content',
    '#rdm-tests-results-list'], function(i, dom) {
    $(dom).html('');
  });
  $('#rdm-tests-results-summary-filter-by_state').val('All');
  for (definition in RDMTests.TEST_RESULTS) {
    delete RDMTests.TEST_RESULTS[definition];
  }
};


/**
 * Adds CSS class to the DOM based on the given state.
 * This CSS class usually adds appropriate color to the text in the DOM.
 * @param {String} state A valid test result state
 * -- 'Passed', 'Failed', 'Broken', 'Not Run'.
 * @param {Object} dom A jQuery object representation of the DOM
 * to which the class needs to be added.
 */
RDMTests.prototype.add_state_class = function(state, dom) {
  $(dom).removeClass($(dom).attr('class'));
  switch (state) {
    case 'Passed':
      $(dom).addClass('test-state-passed');
      break;
    case 'Failed':
      $(dom).addClass('test-state-failed');
      break;
    case 'Broken':
      $(dom).addClass('test-state-broken');
      break;
    case 'Not Run':
      $(dom).addClass('test-state-not_run');
      break;
  }
};


/**
 * Initializes all the result screen DOMs and displays the results.
 * @param {Object} results The response Object from the RDM Tests Server.
 * which contains results.
 */
RDMTests.prototype.display_results = function(results) {
  $('#tests_control_frame').hide();
  rdmtests.reset_results();

  $('#rdm-tests-results-uid').html(results['UID']);

  for (key in results['stats']) {
    $('#rdm-tests-results-stats-figures')
    .append($('<td />').html(results['stats'][key]));
  }

  //Summary of results by category
  for (key in results['stats_by_catg']) {
    var passed = results['stats_by_catg'][key]['passed'];
    var total = results['stats_by_catg'][key]['total'];
    if (total != 0) {
      var percent = '&nbsp;&nbsp;(' +
                    Math.ceil(passed / total * 100).toString() +
                    '%) </span>';
    } else {
      var percent = '&nbsp;&nbsp;- </span>';
    }

    $('#rdm-tests-results-summary-by_catg-content')
    .append($('<li />')
    .html('<span>' +
        key +
        '</span>' +
        '<span class="stats_by_catg">' +
        passed.toString() +
        '&nbsp;/&nbsp;' +
        total.toString() +
        percent));
  }

  var number_of_warnings = 0;
  var number_of_advisories = 0;

  for (index in results['test_results']) {
    var warnings = results['test_results'][index]['warnings'];
    var advisories = results['test_results'][index]['advisories'];
    var definition = results['test_results'][index]['definition'];
    var state = results['test_results'][index]['state'];

    //Populating a global variable with test results for faster lookups
    RDMTests.TEST_RESULTS[definition] = results['test_results'][index];

    number_of_warnings += warnings.length;
    for (var i = 0; i < warnings.length; i++) {
      $('#rdm-tests-results-warnings-content')
      .append($('<li />')
      .html(definition + ': ' + warnings[i]));
    }
    number_of_advisories += advisories.length;
    for (var i = 0; i < advisories.length; i++) {
      $('#rdm-tests-results-advisories-content')
      .append($('<li />')
      .html(definition + ': ' + advisories[i]));
    }
    var test_option = $('<option />').val(definition).text(definition);

    rdmtests.add_state_class(state, test_option);

    $('#rdm-tests-results-list').append(test_option);
  }

  //Populate the filter with test categories
  $('#rdm-tests-results-summary-filter-by_catg')
  .append($('<option />')
  .val('All')
  .html('All'));

  rdmtests.query_server('/GetTestCategories', {}, function(data) {
    for (var i = 0; i < data['Categories'].length; i++) {
      $('#rdm-tests-results-summary-filter-by_catg')
      .append($('<option />')
      .val(data['Categories'][i])
      .html(data['Categories'][i]));
    }
  });

  //Update the Warnings and Advisories counter
  $('#rdm-tests-results-warning-count').html(number_of_warnings.toString());
  $('#rdm-tests-results-advisory-count').html(number_of_advisories.toString());

  $('#rdm-tests-results-list').change(function() {
    rdmtests.result_list_changed();
  });

  // select the first item
  $('#rdm-tests-results-list').val(results['test_results'][0]['definition']);
  rdmtests.result_list_changed();
  $('#rdm-tests-results').show();

  //Hide/Show Download Logs button
  if (results['logs_disabled'] == true) {
    $('#rdm-tests-results-button-download').hide();
  } else {
    $('#rdm-tests-results-button-download').show();
  }
};


/**
 * This is triggered when a definition in results summary is selected.
 * The corresponding 'doc', 'debug' and category is updated in the info div.
 */
RDMTests.prototype.result_list_changed = function() {
  var definition = $('#rdm-tests-results-list option:selected').text();
  var state = RDMTests.TEST_RESULTS[definition]['state'];
  $('#rdm-tests-results-info-title').html(definition);
  rdmtests.add_state_class(state, $('#rdm-tests-results-info-state')
  .html(state));

  $('#rdm-tests-results-info-catg')
  .html(RDMTests.TEST_RESULTS[definition]['category']);

  $('#rdm-tests-results-info-doc')
  .html(RDMTests.TEST_RESULTS[definition]['doc']);

  var debug = RDMTests.TEST_RESULTS[definition]['debug'];
  $('#rdm-tests-results-info-debug').html(debug.join('<br />'));
};


/**
 * Validates the user input on Test Control Frame upon submission.
 * Checks for proper threshold values and other parameters.
 * @return {Boolean} True on successful validation.
 */
RDMTests.prototype.validate_form = function() {
  this.isNumberField = function(dom) {
    var value = $(dom).val();
    if (value === '') {
      return true;
    }
    if (isNaN(parseFloat(value)) || !isFinite(value)) {
      alert(
            $(dom).attr('id').replace('_', ' ').toUpperCase() +
            ' must be a number!'
      );
      return false;
    } else {
      return true;
    }
  };

  if ($('#devices_list option').size() < 1) {
    alert('There are no devices patched to the selected universe!');
    return false;
  }

  if (!(this.isNumberField($('#write_delay')) &&
        this.isNumberField($('#dmx_frame_rate')) &&
        this.isNumberField($('#slot_count')))) {
    return false;
  }

  var slot_count_val = parseFloat($('#slot_count').val());
  if (slot_count_val < 1 || slot_count_val > 512) {
    alert('Invalid number of slots (expected: [1-512])');
    return false;
  }

  var test_filter = ['all'];

  if ($('#rdm-tests-selection-subset').attr('checked')) {
    if ($('select[name="subset_test_defs"]').val() == null) {
      alert('No tests were selected!');
      return false;
    } else {
      test_filter = $('select[name="subset_test_defs"]').val();
    }
  } else if ($('#rdm-tests-selection-previously_failed').attr('checked')) {
    if ($('select[name="failed_test_defs"]').val() == null) {
      alert('Select failed tests to run again!');
      return false;
    } else {
      test_filter = $('select[name="failed_test_defs"]').val();
    }
  }
  rdmtests.run_tests(test_filter);
};


$(document).ready(function() {
  rdmtests.bind_events_to_doms();
  rdmtests.update_universe_list();
  rdmtests.fetch_test_defs();
});
